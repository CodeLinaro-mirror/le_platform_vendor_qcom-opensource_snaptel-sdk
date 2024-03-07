/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AudioPlayerImpl_HPP
#define AudioPlayerImpl_HPP

#include <queue>
#include <cstdio>
#include <mutex>

#include "common/AsyncTaskQueue.hpp"

#include <telux/audio/AudioPlayer.hpp>
#include <telux/audio/AudioListener.hpp>
#include <telux/audio/AudioManager.hpp>

namespace telux {
namespace audio {

class GetVolumeResponseListener {
 public:
    GetVolumeResponseListener(std::mutex& streamMtx);
    bool responseReady = false;
    StreamVolume volume;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void getVolumeCompletion(StreamVolume volume, telux::common::ErrorCode errorCode);
 private:
    std::mutex &streamMutex_;
};

class SetVolumeResponseListener {
 public:
    SetVolumeResponseListener(std::mutex& streamMtx);
    bool responseReady = false;
    telux::common::ErrorCode errorCode;
    std::condition_variable cv;
    void setVolumeCompletion(telux::common::ErrorCode errorCode);
 private:
    std::mutex &streamMutex_;
};

class AudioPlayerImpl : public IAudioPlayer,
                        public IPlayListener,
                        public IAudioListener,
                        public std::enable_shared_from_this<AudioPlayerImpl> {

 public:
    AudioPlayerImpl(std::shared_ptr<IAudioManager> audioManager);
    ~AudioPlayerImpl();

    telux::common::ErrorCode startPlayback(
        StreamConfig streamConfig,
        std::vector<PlaybackFile> &filesToPlay,
        std::weak_ptr<IPlayListListener> statusListener) override;

    telux::common::ErrorCode stopPlayback() override;

    telux::common::ErrorCode setVolume(StreamVolume volume) override;

    telux::common::ErrorCode getVolume(StreamVolume &volume) override;

    void onReadyForWrite() override;
    void onPlayStopped() override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;

    void play();

    void createStreamCompletion(std::shared_ptr<IAudioStream> &stream,
        telux::common::ErrorCode result);

    void deleteStreamCompletion(telux::common::ErrorCode result);

    void writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void stopAudioCompletion(telux::common::ErrorCode result);

    WriteResponseCb writeCompleteCb_;

 private:
    /* Number of buffers used for playback */
    const size_t BUFFER_POOL_SIZE = 2;

    /* Time in seconds for which player thread waits for the response from audio server */
    const int TIME_10_SECONDS = 10;

    bool isCompressed_ = false;
    bool isAdspWriteReady_ = true;
    bool hasSsrOccurred_ = false;
    bool isFileOpened_ = false;
    bool isStreamOpened_ = false;
    bool isPlayInProgress_ = false;
    bool hasUserRequestedStop_ = false;
    bool isCreateResponseReady_ = false;
    bool isDeleteResponseReady_ = false;
    bool isStopResponseReady_ = false;
    bool isStopAudioReady_ = false;
    uint32_t bufferSize_ = 0;

    FILE *curFile_;
    std::string curFileName_;
    StreamConfig streamConfig_;
    telux::common::Status status_;
    std::mutex writeMtx_;
    std::mutex playerMtx_;
    std::mutex streamMtx_;
    std::vector<PlaybackFile> playbackFiles_;
    std::condition_variable adspReady_;
    std::condition_variable asyncResponse_;
    std::condition_variable bufferAvailable_;
    std::condition_variable compressedPlayStopped_;
    telux::common::ErrorCode errToReport_;

    std::weak_ptr<IPlayListListener> statusListener_;
    std::shared_ptr<IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> bufferPool_;
    telux::common::AsyncTaskQueue<void> asyncTaskQ_;

    telux::common::ErrorCode initAudioStream();
    telux::common::ErrorCode deinitAudioStream();
    telux::common::ErrorCode initFileToPlay();
    telux::common::ErrorCode deinitFileToPlay();
    telux::common::ErrorCode prepareBuffers();
    telux::common::ErrorCode playAudioSamples();
    telux::common::ErrorCode finalizePlayback();
    telux::common::ErrorCode finalizeCompressedPlayback();
    telux::common::ErrorCode registerForEvents();
    telux::common::ErrorCode deregisterForEvents();
    telux::common::ErrorCode waitAllWriteResponse();
    telux::common::ErrorCode setFormatAndOffset(
        AudioFormat audioFormat, long &contentOffset);
    telux::common::ErrorCode adjustFileAndState(long contentOffset);

    void resetState();
    void terminatePlayback();
    void reportError(telux::common::ErrorCode ec, std::string file);
    void reportPlayed();
    void reportPlaybackFinished();
    void reportPlaybackStarted();
    void reportPlaybackStopped();
    void unblockPlayerThread(bool setSSRStatus);
};

}  // end of namespace audio
}  // end of namespace telux

#endif // AudioPlayerImpl_HPP