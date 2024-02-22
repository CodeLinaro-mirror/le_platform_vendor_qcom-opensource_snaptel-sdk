/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <errno.h>
#include <thread>
#include <future>

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "AudioPlayerImpl.hpp"

namespace telux {
namespace audio {

/*
 * Represents possible states for the state machine.
 */
enum class PlayerState {

    /*
     * Resources are allocated and initialized. For example; creating audio stream,
     * allocating ping-pong buffers and registering for SSR events.
     */
    INIT_PLAYER,

    /*
     * Represents heart of the player. Based on how to play the file (skip,
     * count, indefinite), decision is made to skip or schedule the file for
     * playback.
     */
    SELECT_FILE_TO_PLAY,

    /*
     * File to play is opened from the application specified path.
     */
    INIT_FILE,

    /*
     * Audio sample are actually played.
     */
    PLAYING,

    /*
     * Final steps are taken to conclude playback of the file currently played.
     * For example; play the last 2 buffers, handle errors as applicable and stop
     * the compressed stream.
     */
    FILE_PLAY_END,

    /*
     * Application is informed that a particular file has been played successfully.
     */
    REPORT_PLAYED,

    /*
     * File currently played is closed after it has been played completely.
     */
    DEINIT_FILE,

    /*
     * Marks graceful completion of the whole playback. Stream is closed, resources
     * are released, application is informed that playback is completed successfully.
     */
    REPORT_FINISH,

    /*
     * Reached whenever an error is encountered during playback. Application is informed
     * that an error has occurred.
     */
    REPORT_ERROR,

    /*
     * Represents a fatal error situation. Stream is closed and resources are released.
     * Player thread is terminated.
     */
    TERMINATE
};

AudioPlayerImpl::AudioPlayerImpl(std::shared_ptr<IAudioManager> audioManager) {

    audioManager_ = audioManager;
}

AudioPlayerImpl::~AudioPlayerImpl() {
    LOG(DEBUG, __FUNCTION__);

    /*
     * It is not expected that an application releases AudioPlayerImpl instance
     * during an ongoing playback. But if it happens, trigger cleanup and exit.
     */
    hasUserRequestedStop_ = true;

    unblockPlayerThread(false);

    /*
     * Ensure all background threads are terminated before releasing AudioPlayerImpl
     * fully to maintain correct order of destruction and cleanup.
     */
    asyncTaskQ_.shutdown();
}

/*
 * Places a request to start playback.
 */
telux::common::ErrorCode AudioPlayerImpl::startPlayback(
        StreamConfig streamConfig,
        std::vector<PlaybackFile> &filesToPlay,
        std::weak_ptr<IPlayListListener> statusListener) {

   {
    std::lock_guard<std::mutex> lock(playerMtx_);

    if (isPlayInProgress_) {
        /* A playback is already running */
        LOG(ERROR, __FUNCTION__, " playback in progress");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (filesToPlay.empty()) {
        LOG(ERROR, __FUNCTION__, " empty files list");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    streamConfig_ = streamConfig;
    statusListener_ = statusListener;
    playbackFiles_ = filesToPlay;

    resetState();

    /* Player thread */
    auto f = std::async(std::launch::async, [this]() { this->play(); }).share();
    asyncTaskQ_.add(f);

    isPlayInProgress_ = true;

    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 * Places a request to terminate the playback.
 */
telux::common::ErrorCode AudioPlayerImpl::stopPlayback() {
   {
    std::lock_guard<std::mutex> playerLock(playerMtx_);

    if (!isPlayInProgress_) {
        LOG(ERROR, __FUNCTION__, " no playback running");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    hasUserRequestedStop_ = true;

    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 * Reset internal state since application may start playback again
 * without releasing the current IAudioPlayer instance.
 */
void AudioPlayerImpl::resetState() {

    bufferSize_ = 0;
    curFileName_ = "";
    bufferPool_ = {};
    curFile_ = nullptr;
    isFileOpened_ = false;
    hasSsrOccurred_ = false;
    isStreamOpened_ = false;
    isCompressed_ = false;
    isAdspWriteReady_ = true;
    isStopAudioReady_ = false;
    hasUserRequestedStop_ = false;
    isStopResponseReady_ = false;
    isCreateResponseReady_ = false;
    isDeleteResponseReady_ = false;
    errToReport_ = telux::common::ErrorCode::SUCCESS;
}

/*
 * Identify how many bytes to skip in the given file before passing audio data to the PAL.
 * Multi-channel ("#!AMR_MC1.0\n"/"#!AMR-WB_MC1.0\n") playback is not supported.
 */
telux::common::ErrorCode AudioPlayerImpl::setFormatAndOffset(AudioFormat audioFormat,
        long &contentOffset) {

    switch (audioFormat) {
        case AudioFormat::AMRWB_PLUS:
            isCompressed_ = true;
            /* As per ETSI TS 126 290 V8.0.0 (2009-01) section 8.3 */
            contentOffset = 2;
            break;
        case AudioFormat::AMRWB:
            isCompressed_ = true;
            /* First 9 bytes in the file header are #!AMR-WB\n as per RFC4867 */
            contentOffset = 9;
            break;
        case AudioFormat::AMRNB:
            isCompressed_ = true;
            /* First 6 bytes in the file header are #!AMR\n as per RFC4867 */
            contentOffset = 6;
            break;
        case AudioFormat::PCM_16BIT_SIGNED:
            isCompressed_ = false;
            /* Every byte is data byte */
            contentOffset = 0;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid fmt ", static_cast<int>(audioFormat));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Directs what to play and how.
 */
void AudioPlayerImpl::play() {

    int ret = 0;
    long contentOffset = 0;
    uint32_t curFileIdx = 0;
    uint32_t numTimesFilePlayed = 0;

    PlaybackFile curPbFile;
    PlayerState nextState = PlayerState::INIT_PLAYER;
    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;

    while(!hasUserRequestedStop_ && !hasSsrOccurred_) {
        switch (nextState) {
            case PlayerState::INIT_PLAYER:
                reportPlaybackStarted();

                ec = initAudioStream();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                ec = prepareBuffers();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                ec = setFormatAndOffset(streamConfig_.format, contentOffset);
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                ec = registerForEvents();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                writeCompleteCb_ = std::bind(&AudioPlayerImpl::writeCompletion, this,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

                nextState = PlayerState::SELECT_FILE_TO_PLAY;
                break;

            case PlayerState::SELECT_FILE_TO_PLAY:
                if (curFileIdx >= playbackFiles_.size()) {
                    /* All files played successfully */
                    nextState = PlayerState::REPORT_FINISH;
                    break;
                }

                curPbFile = playbackFiles_[curFileIdx];

                if (curPbFile.repeatInfo.type == RepeatType::COUNT) {
                    if (curPbFile.repeatInfo.count == 0) {
                        /* Zero count is same as skip */
                        ++curFileIdx;
                    } else if (numTimesFilePlayed == 0) {
                        /* First time playing this file */
                        curFileName_ = curPbFile.absoluteFilePath;
                        nextState = PlayerState::INIT_FILE;
                    } else if (numTimesFilePlayed < curPbFile.repeatInfo.count) {
                        /* Play this file again */
                        ++numTimesFilePlayed;
                        nextState = PlayerState::PLAYING;
                    } else {
                        /* File has been played for the given iterations */
                        numTimesFilePlayed = 0;
                        ++curFileIdx;
                        nextState = PlayerState::DEINIT_FILE;
                    }
                } else if (curPbFile.repeatInfo.type == RepeatType::SKIP) {
                    /* Skip this file */
                    ++curFileIdx;
                } else {
                    /* Play the file indefinitely */
                    if (!isFileOpened_) {
                        /* Playing file for the very first time, open it */
                        curFileName_ = curPbFile.absoluteFilePath;
                        nextState = PlayerState::INIT_FILE;
                    } else {
                        /*
                         * Playing file for the next iteration.
                         * Move the file position indicator to the beginning of the
                         * given file stream and clear end-of-file and error indicators.
                         */
                        std::rewind(curFile_);
                        nextState = PlayerState::PLAYING;
                    }
                }
                break;

            case PlayerState::INIT_FILE:
                ec = initFileToPlay();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    errToReport_ = ec;
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                /* Discard header for the compressed format file as expected by PAL */
                ret = std::fseek(curFile_, contentOffset, SEEK_CUR);
                if (ret) {
                    LOG(ERROR, __FUNCTION__, " can't fseek");
                    errToReport_ = telux::common::ErrorCode::SYSTEM_ERR;
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                nextState = PlayerState::PLAYING;
                break;

            case PlayerState::PLAYING:
                ec = playAudioSamples();
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    nextState = PlayerState::REPORT_ERROR;
                    break;
                }

                if (std::feof(curFile_)) {
                    nextState = PlayerState::FILE_PLAY_END;
                }
                break;

            case PlayerState::FILE_PLAY_END:
                ec = finalizePlayback();
                if (ec == telux::common::ErrorCode::SUCCESS) {
                    nextState = PlayerState::REPORT_PLAYED;
                } else if (ec == telux::common::ErrorCode::REQUEST_RATE_LIMITED) {
                    /*
                     * ADSP pipeline is full, need to resend same buffer again once
                     * ADSP is ready to accept next buffer.
                     */
                    nextState = PlayerState::PLAYING;
                } else {
                    nextState = PlayerState::REPORT_ERROR;
                }
                break;

            case PlayerState::REPORT_PLAYED:
                reportPlayed();

                if (curPbFile.repeatInfo.type == RepeatType::COUNT) {
                    ++numTimesFilePlayed;
                }

                nextState = PlayerState::SELECT_FILE_TO_PLAY;
                break;

            case PlayerState::DEINIT_FILE:
                deinitFileToPlay();
                nextState = PlayerState::SELECT_FILE_TO_PLAY;
                break;

            case PlayerState::REPORT_ERROR:
                reportError(errToReport_, curFileName_);
                nextState = PlayerState::TERMINATE;
                break;

            case PlayerState::REPORT_FINISH:
               {
                std::lock_guard<std::mutex> playerLock(playerMtx_);

                if (hasUserRequestedStop_) {
                    /* User placed request to stop playing before we can finish playback */
                    break;
                }

                /*
                 * First cleanup internal state and then report to maintain
                 * correct order of execution and sanity of variables.
                 */
                deinitAudioStream();
                deregisterForEvents();
                bufferPool_ = {};
                isPlayInProgress_ = false;
                hasUserRequestedStop_ = false;
                reportPlaybackFinished();
                return;
               }

            case PlayerState::TERMINATE:
                terminatePlayback();
                reportPlaybackStopped();
                return;

            default:
                LOG(ERROR, __FUNCTION__, " invalid state ", static_cast<int>(nextState));
                errToReport_ = telux::common::ErrorCode::INVALID_STATE;
                nextState = PlayerState::REPORT_ERROR;
                break;
        }
    }

    /*
     * (1) An error occurs, TERMINATE state is entered, cleanup is done, onPlaybackStopped()
     * is called. Player thread is pre-empted. Application calls stopPlayback() which sets
     * hasUserRequestedStop_. Since we return from TERMINATE state, player thread will not
     * execute below code. Therefore, there is no race between application and player thread.
     * (2) An error occurs, player thread is pre-empted. Application calls stopPlayback()
     * which sets hasUserRequestedStop_. While loop breaks, and code below will be executed.
     * TERMINATE state is never entered. Therefore, no race between application and player
     * thread.
     * (3) Application calls startPlay() immediately followed by stopPlay() such that player
     * thread doesn't get chance to execute state machine (enter while loop). When the player
     * thread is scheduled control reaches here and termination occurs as expected. Check for
     * valid object/pointer/value maintains sanity of the cleanup.
     */
    terminatePlayback();
    deregisterForEvents();
    reportPlaybackStopped();
}

/*
 * Terminate playback completely.
 */
void AudioPlayerImpl::terminatePlayback() {
   {
    /*
     * Protect from player thread terminating playback and application giving up
     * this class instance. Also prevents against accessing invalid variables,
     * objects and pointers.
     */
    std::lock_guard<std::mutex> playerLock(playerMtx_);

    if (!isPlayInProgress_) {
        return;
    }

    deinitFileToPlay();

    if (!hasSsrOccurred_) {
        waitAllWriteResponse();
        deinitAudioStream();
    }

    bufferPool_ = {};
    isPlayInProgress_ = false;
    hasUserRequestedStop_ = false;
   }
}

/*
 * When an async request to create stream is sent to the audio server, WAIT_TIME second
 * timeout is used to ensure that the player thread doesn't get stuck forever waiting
 * for async response from server.
 *
 * CreateStreamResponseCb callback must be valid/existent in memory if the response
 * comes after this timeout. Therefore, define a method whose lifetime is more than
 * that of the AudioManagerImpl.
 */
void AudioPlayerImpl::createStreamCompletion(std::shared_ptr<IAudioStream> &stream,
        telux::common::ErrorCode result) {
   {
    std::lock_guard<std::mutex> streamLock(streamMtx_);

    if (result == telux::common::ErrorCode::SUCCESS) {
      audioPlayStream_ = std::dynamic_pointer_cast<telux::audio::IAudioPlayStream>(stream);
      isStreamOpened_ = true;
    }

    errToReport_ = result;
    isCreateResponseReady_ = true;
    asyncResponse_.notify_all();
   }
}

/*
 * Creates an audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::initAudioStream() {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::common::Status status;

    auto createStreamResponseCb = std::bind(&AudioPlayerImpl::createStreamCompletion,
        this, std::placeholders::_1, std::placeholders::_2);

    status = audioManager_->createStream(streamConfig_, createStreamResponseCb);
    if (status != telux::common::Status::SUCCESS) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " failed create stream ", static_cast<int>(ec));
        return ec;
    }

   {
    std::unique_lock<std::mutex> streamLock(streamMtx_);

    /*
     * When an async request to create stream is sent to the audio server,
     * we don't know whether the response from server will come or not in
     * error scenarios for example, SSR. If it comes, how much time it will
     * take. Use a WAIT_TIME second timeout to prevent player thread from
     * remaining blocked forever if response doesn't come.
     */
    waitResult = asyncResponse_.wait_for(streamLock,
        std::chrono::seconds(WAIT_TIME),
        [=] { return (isCreateResponseReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timedout");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't create stream ", static_cast<int>(errToReport_));
        return errToReport_;
    }

    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 * Receives response of the delete stream.
 */
void AudioPlayerImpl::deleteStreamCompletion(telux::common::ErrorCode result) {
   {
    std::lock_guard<std::mutex> streamLock(streamMtx_);

    errToReport_ = result;
    isDeleteResponseReady_ = true;
    asyncResponse_.notify_all();
   }
}

/*
 * Deletes audio stream.
 */
telux::common::ErrorCode AudioPlayerImpl::deinitAudioStream() {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::common::Status status;

    if (!isStreamOpened_ || !audioPlayStream_) {
        /* Stream already closed or doesn't exist */
        LOG(ERROR, __FUNCTION__, " no stream");
        return telux::common::ErrorCode::SUCCESS;
    }

    auto deleteStreamResponseCb = std::bind(&AudioPlayerImpl::deleteStreamCompletion,
        this, std::placeholders::_1);

    status = audioManager_-> deleteStream(audioPlayStream_, deleteStreamResponseCb);
    if (status != telux::common::Status::SUCCESS) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " failed delete stream ", static_cast<int>(ec));
        return ec;
    }

   {
    std::unique_lock<std::mutex> streamLock(streamMtx_);

    waitResult = asyncResponse_.wait_for(streamLock,
        std::chrono::seconds(WAIT_TIME),
        [=] { return (isDeleteResponseReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timedout");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't delete stream ", static_cast<int>(errToReport_));
        return errToReport_;
    }

    isStreamOpened_ = false;
    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 * Allocate buffers for writing audio samples.
 */
telux::common::ErrorCode AudioPlayerImpl::prepareBuffers() {

    bufferSize_ = 0;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    for (size_t x = 0; x < BUFFER_POOL_SIZE; x++) {

        /* Allocate 2 buffers (ping-pong) and cache buffer pointers */
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            LOG(ERROR, __FUNCTION__, " can't allocate buffers");
            return telux::common::ErrorCode::NO_MEMORY;
        }

        bufferPool_.push(streamBuffer);

        /*
         * Identify optimal buffer size for this stream type and set it.
         * The getMinSize() gives optimal buffer size for given stream (use case).
         * If it doesn't give us any value, set buffer size to the maximum possible
         * value to minimize playback latency.
         */
        bufferSize_ = streamBuffer->getMinSize();
        if (!bufferSize_) {
            bufferSize_ =  streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(bufferSize_);
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Opens a file for playback from the user specified path.
 */
telux::common::ErrorCode AudioPlayerImpl::initFileToPlay() {

    if (curFileName_.empty()) {
        LOG(ERROR, __FUNCTION__, " missing file name");
        return telux::common::ErrorCode::MISSING_RESOURCE;
    }

    curFile_ = std::fopen(curFileName_.c_str(), "r");
    if (!curFile_) {
        LOG(ERROR, __FUNCTION__, " can't open file, err ", static_cast<int>(errno));
        return telux::common::ErrorCode::NO_MEMORY;
    }

    isFileOpened_ = true;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Closes the file.
 */
telux::common::ErrorCode AudioPlayerImpl::deinitFileToPlay() {

    int ret;

    /*
     * 1. File closed when in DEINIT_FILE state.
     * 2. Explicit stop playback request received, termination sequence started.
     * 3. This method is called again.
     * Since curFile_ becomes invalid after step 1, prevent accessing it otherwise
     * crash occurs.
     */
    if (!isFileOpened_ || !curFile_) {
        LOG(ERROR, __FUNCTION__, " no opened file");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ret = std::fclose(curFile_);
    if (ret) {
        LOG(ERROR, __FUNCTION__, " can't close file");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    isFileOpened_ = false;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Actually sends samples to the server for playback.
 */
telux::common::ErrorCode AudioPlayerImpl::playAudioSamples() {

    bool waitResult = false;
    std::cv_status waitResultNoPredicate{};
    uint32_t numBytesRead = 0;
    telux::common::ErrorCode ec;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

   {
    std::unique_lock<std::mutex> writeLock(writeMtx_);

    if(bufferPool_.empty()) {
        /* Wait for a free buffer. Predicate is not used because of ping-pong */
        waitResultNoPredicate = bufferAvailable_.wait_for(writeLock,
            std::chrono::seconds(WAIT_TIME));

        if (waitResultNoPredicate == std::cv_status::timeout) {
            LOG(ERROR, __FUNCTION__, " timedout");
            errToReport_ = telux::common::ErrorCode::OPERATION_TIMEOUT;
            return errToReport_;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            errToReport_ = telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
            return errToReport_;
        }

        if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
            /* Error encountered while playing previously sent buffer, bail out */
            return errToReport_;
        }
    }

    if(isCompressed_ && !isAdspWriteReady_) {
        /* Although buffer is available but ADSP can't accept at the moment */
        waitResult = adspReady_.wait_for(writeLock,
            std::chrono::seconds(WAIT_TIME),
            [=] { return (isAdspWriteReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

            if (!waitResult) {
                LOG(ERROR, __FUNCTION__, " timedout");
                errToReport_ = telux::common::ErrorCode::OPERATION_TIMEOUT;
                return errToReport_;
            }

            if (hasSsrOccurred_) {
                LOG(ERROR, __FUNCTION__, " ssr occurred");
                errToReport_ = telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
                return errToReport_;
            }

            if (hasUserRequestedStop_) {
                LOG(ERROR, __FUNCTION__, " user stopped");
                errToReport_ = telux::common::ErrorCode::CANCELLED;
                return errToReport_;
            }
    }

    streamBuffer = bufferPool_.front();
    bufferPool_.pop();

    numBytesRead = fread(streamBuffer->getRawBuffer(), 1, bufferSize_, curFile_);

    if ((numBytesRead == 0) && std::feof(curFile_)) {
        /* Complete file has been played */
        bufferPool_.push(streamBuffer);
        return telux::common::ErrorCode::SUCCESS;
    }

    if((numBytesRead != bufferSize_) && !std::feof(curFile_)) {
        /* Can't read requested number of bytes from the file system */
        bufferPool_.push(streamBuffer);
        LOG(ERROR, __FUNCTION__, " can't read file, numBytesRead ", numBytesRead);
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    streamBuffer->setDataSize(numBytesRead);

    status = audioPlayStream_->write(streamBuffer, writeCompleteCb_);
    if(status != telux::common::Status::SUCCESS) {
        bufferPool_.push(streamBuffer);
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " can't write, err ", static_cast<int>(ec));
        return ec;
    }
   }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Response calback to confirm samples played actually or playback failed.
 */
void AudioPlayerImpl::writeCompletion(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode ec) {

    long offset = 0;

    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " write failed, err ", static_cast<int>(ec));
    } else if (buffer->getDataSize() != bytesWritten) {
        /*
         * Whole buffer can't be played successfully,
         * calculate how many bytes (offset) we need to play again and rewind to it.
         */
        offset = (-1) * (static_cast<long>((buffer->getDataSize() - bytesWritten)));
        std::fseek(curFile_, offset, SEEK_CUR);
    } else {
        /* Success, send next buffer to play */
    }

   {
    std::lock_guard<std::mutex> writeLock(writeMtx_);

    /* Let the player thread know play success/failure for this buffer */
    errToReport_ = ec;

    if (isCompressed_ && offset) {
        /* ADSP pipeline can't accept more buffers to play at the moment */
        isAdspWriteReady_ = false;
    }

    bufferPool_.push(buffer);

    bufferAvailable_.notify_all();
   }
}

/*
 * Wait for responses to all the write request sent to the audio server
 * till now. This ensures writeCompletion() exist in memory till the time
 * it will be accessed to deliver the write results from the server.
 */
telux::common::ErrorCode AudioPlayerImpl::waitAllWriteResponse() {

    std::cv_status waitResult{};

   {
    std::unique_lock<std::mutex> bufferWaitLock(writeMtx_);

    while(bufferPool_.size() != BUFFER_POOL_SIZE) {
        /*
         * Predicate is not used since there are two writes active at any time
         * instant and response of latest write will overwrite response of
         * previous write. Therefore, practically, predicate will be true always
         * and will not server its actual purpose.
         */
        waitResult = bufferAvailable_.wait_for(bufferWaitLock,
            std::chrono::seconds(WAIT_TIME));

        if (waitResult == std::cv_status::timeout) {
            LOG(ERROR, __FUNCTION__, " timedout");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }

        if (hasSsrOccurred_) {
            LOG(ERROR, __FUNCTION__, " ssr occurred");
            return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 * Updates application that an error has occurred.
 */
void AudioPlayerImpl::reportError(telux::common::ErrorCode ec, std::string file) {

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onError(ec, file);
    }
}

/*
 *  Updates application that the file has been played.
 */
void AudioPlayerImpl::reportPlayed() {

    auto playListListener = statusListener_.lock();
    if (playListListener) {
      playListListener->onFilePlayed(curFileName_);
    }
}

/*
 *  Updates application that all the files given have been played in the
 *  manner specified by the application.
 */
void AudioPlayerImpl::reportPlaybackFinished() {

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onPlaybackFinished();
    }
}

/*
 *  Updates application that the playback is started.
 */
void AudioPlayerImpl::reportPlaybackStarted() {

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onPlaybackStarted();
    }
}

/*
 *  Updates application that the playback is terminated.
 */
void AudioPlayerImpl::reportPlaybackStopped() {

    auto playListListener = statusListener_.lock();
    if (playListListener) {
        playListListener->onPlaybackStopped();
    }
}

/*
 * Player sends last 2 buffers (ping-pong) to the audio server for playback.
 * Handle below possible cases:
 *
 *  Case | Buffer 1 | Buffer 2
 * ---------------------------
 *   1     Played     Played
 *   2     Failed     Failed
 *   3     Failed     Played
 *   4     Played     Failed
 */
telux::common::ErrorCode AudioPlayerImpl::finalizePlayback() {

    bool finalizeAMR = false;
    telux::common::ErrorCode ec;

    /* For all cases wait for the response from server for both the buffers */
    ec = waitAllWriteResponse();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return ec;
    }

   {
    std::unique_lock<std::mutex> writeLock1(writeMtx_);

    /* Case 1 */
    if ((errToReport_ == telux::common::ErrorCode::SUCCESS) && isAdspWriteReady_) {
        if (!isCompressed_) {
            /* PCM playback completed successfully */
            return telux::common::ErrorCode::SUCCESS;
        }
        finalizeAMR = true;
    }
   }

   if (finalizeAMR) {
       return finalizeCompressedPlayback();
   }

   {
    std::unique_lock<std::mutex> writeLock2(writeMtx_);
    /*
     * Case 2,3,4.
     * As per the current design, audio server's response to the last write overwrites
     * response to the 2nd last write call. If a real error occurred return it. If the
     * ADSP couldn't play buffer's, inform player thread to resend them.
     */
    if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
        /* Error occurred while trying to play the last two buffers */
        return errToReport_;
    }

    if (!isAdspWriteReady_) {
        /* Need to resend the buffer */
        return telux::common::ErrorCode::REQUEST_RATE_LIMITED;
    }

    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 * Receives response of the stop audio for compressed playback.
 */
void AudioPlayerImpl::stopAudioCompletion(telux::common::ErrorCode result) {
   {
    std::lock_guard<std::mutex> streamLock(streamMtx_);

    errToReport_ = result;
    isStopResponseReady_ = true;
    asyncResponse_.notify_all();
   }
}

/*
 * When playing AMR formatted audio, ADSP needs to be instructed to stop after
 * playing all the pending buffers it has in the pipeline.
 */
telux::common::ErrorCode AudioPlayerImpl::finalizeCompressedPlayback() {

    bool waitResult = false;
    telux::common::ErrorCode ec;
    telux::common::Status status;

    auto stopAudioResponseCb = std::bind(&AudioPlayerImpl::stopAudioCompletion,
        this, std::placeholders::_1);

    status = audioPlayStream_->stopAudio(StopType::STOP_AFTER_PLAY, stopAudioResponseCb);
    if (status != telux::common::Status::SUCCESS) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " failed stop, err ", static_cast<int>(ec));
        return ec;
    }

   {
    std::unique_lock<std::mutex> streamLock(streamMtx_);

    waitResult = asyncResponse_.wait_for(streamLock,
        std::chrono::seconds(WAIT_TIME),
        [=] { return (isStopResponseReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timedout");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    if (errToReport_ != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't stop stream ", static_cast<int>(errToReport_));
        return errToReport_;
    }
   }

   {
    std::unique_lock<std::mutex> writeLock(writeMtx_);

    /* Wait for response from ADSP confirming it stopped (drain done) */
    waitResult = compressedPlayStopped_.wait_for(writeLock,
        std::chrono::seconds(WAIT_TIME),
        [=] { return (isStopAudioReady_ || hasSsrOccurred_ || hasUserRequestedStop_); });

    if (!waitResult) {
        LOG(ERROR, __FUNCTION__, " timedout");
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    }

    if (hasSsrOccurred_) {
        LOG(ERROR, __FUNCTION__, " ssr occurred");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    if (hasUserRequestedStop_) {
        LOG(ERROR, __FUNCTION__, " user stopped");
        return telux::common::ErrorCode::CANCELLED;
    }

    /* Compressed playback completed successfully */
    return telux::common::ErrorCode::SUCCESS;
   }
}

/*
 *  Registers for async events from the audio server.
 */
telux::common::ErrorCode AudioPlayerImpl::registerForEvents() {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    if (isCompressed_) {
        /* Register for onReadyForWrite() and onPlayStopped() callbacks */
        status = audioPlayStream_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            ec = telux::common::CommonUtils::toErrorCode(status);
            LOG(ERROR, __FUNCTION__,
                " can't register compresscb, err ", static_cast<int>(ec));
            return ec;
        }
    }

    /* Register for SSR onServiceStatusChange() callback */
    status = audioManager_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " can't register ssrcb, err ", static_cast<int>(ec));
        if (isCompressed_) {
            audioPlayStream_->deRegisterListener(shared_from_this());
        }
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 *  Deregisters for async events from the audio server.
 */
telux::common::ErrorCode AudioPlayerImpl::deregisterForEvents() {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    status = audioManager_->deRegisterListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        /* Don't treat as fatal */
        ec = telux::common::CommonUtils::toErrorCode(status);
        LOG(ERROR, __FUNCTION__, " can't deregister ssrcb, err ", static_cast<int>(ec));
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 *  Called when the audio server can accept the next buffer for compressed play.
 */
void AudioPlayerImpl::onReadyForWrite() {

   {
    std::lock_guard<std::mutex> writeLock(writeMtx_);

    isAdspWriteReady_ = true;
    adspReady_.notify_all();
   }
}

/*
 *  Called to confirm all buffers of compressed playback have been processed.
 */
void AudioPlayerImpl::onPlayStopped() {

   {
    std::lock_guard<std::mutex> compressStopLock(writeMtx_);

    isStopAudioReady_ = true;
    compressedPlayStopped_.notify_all();
   }
}

/*
 * SSR handling flow:
 * 1. SSR occurs, audio server sends service unavailable.
 * 2. Player thread is unblocked from waits which will never be over now.
 * 3. Player thread does the cleanup, reports play stopped and terminates.
 *
 * The playerMtx_ is used to ensure integrity of the implementation during
 * starting, stopping, playing and destruction. This mutex ensures following
 * five cases remain handled gracefully:
 *
 * 1. App gives up IAudioPlayer instance without actually starting the playback.
 * 2. App gives up IAudioPlayer instance during an on-going playback.
 * 3. SSR occurs during an on-going playback.
 * 4. App calls stopPlayback() explicitly to terminate the playback.
 * 5. Player thread exits due to a fatal error.
 */
void AudioPlayerImpl::onServiceStatusChange(telux::common::ServiceStatus status) {

    LOG(DEBUG, __FUNCTION__, " SSR status ", static_cast<int>(status));

    if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        /* Only service unavailable awareness is needed to exit player thread, just reset state */
        hasSsrOccurred_ = false;
        return;
    }

    unblockPlayerThread(true);
}

/*
 * During playback, player threads waits for async responses from audio server at
 * various times. Unblock player thread so that it can execute next expected step.
 */
void AudioPlayerImpl::unblockPlayerThread(bool setSSRStatus) {
   {
    /*
     * This lock synchronizes player thread, AudioPlayerImpl destruction and
     * caller thread of this method.
     */
    std::lock_guard<std::mutex> playerLock(playerMtx_);
    {
     /* This lock prevents spurious/false wake ups */
     std::lock_guard<std::mutex> ssrLock(writeMtx_);

     if (setSSRStatus) {
        hasSsrOccurred_ = true;
     }

     asyncResponse_.notify_all();
     adspReady_.notify_all();
     bufferAvailable_.notify_all();
     compressedPlayStopped_.notify_all();
    }
   }
}

}  // end of namespace audio
}  // end of namespace telux
