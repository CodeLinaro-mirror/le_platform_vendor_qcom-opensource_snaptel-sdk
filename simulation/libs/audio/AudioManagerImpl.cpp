/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "VoiceStreamImpl.hpp"
#include "PlayStreamImpl.hpp"
#include "CaptureStreamImpl.hpp"
#include "LoopbackStreamImpl.hpp"
#include "ToneGeneratorStreamImpl.hpp"
#include "AudioManagerImpl.hpp"

namespace telux {
namespace audio {

std::atomic<bool> AudioManagerImpl::exitNow_;
std::mutex AudioManagerImpl::serviceStatusGuard_;

AudioManagerImpl::AudioManagerImpl() {
}

AudioManagerImpl::~AudioManagerImpl() {
    std::lock_guard<std::mutex> lock(AudioManagerImpl::serviceStatusGuard_);
    LOG(DEBUG, __FUNCTION__);
    AudioManagerImpl::exitNow_ = true;
}

/*
 * Setup/initiate connection to audio grpc server.
 * Complete non-blocking initializations and schedule blocking ones.
 */
telux::common::Status AudioManagerImpl::init(
        telux::common::InitResponseCb initResultListener) {

    telux::common::Status status;
    std::shared_future<void> future;

    /* Setup AudioGrpcClient */
    transportClient_ = std::make_shared<AudioGrpcClientStub>();
    if (!transportClient_) {
        return telux::common::Status::FAILED;
    }

    status = transportClient_->setup();
    if (status != telux::common::Status::SUCCESS) {
        return status;
    }

    try {
        serviceStatusListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<telux::audio::IAudioListener>>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't setup ListenerManager");
        return telux::common::Status::FAILED;
    }

    /* Schedule blocking initializations */
    future = std::async(std::launch::async, [=]() {
                this->initSync(initResultListener);
             }).share();

    /* Hold onto future's reference until initSync() finishes */
    asyncTaskQueue_.add(future);

    return telux::common::Status::SUCCESS;
}

/*
 * Complete blocking initializations.
 */
void AudioManagerImpl::initSync(telux::common::InitResponseCb initResultListener) {
    LOG(DEBUG, __FUNCTION__);

    bool isSvcReady;
    std::future<bool> future;

    /* Block until connected to the audio server */
    isSvcReady = transportClient_->isReady();
    if (!isSvcReady) {
        future = transportClient_->onReady();
        isSvcReady = future.get();
    }

    transportClient_->registerForServiceStatusEvents(shared_from_this());

    /* Once connected, update local copy of current service state */
    {
      std::lock_guard<std::mutex> lock(serviceStatusGuard_);
      if (AudioManagerImpl::exitNow_) {
          LOG(WARNING, __FUNCTION__, " dropping initSync");
          return;
      }
      if (isSvcReady) {
          serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
      } else {
          serviceCurrentStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
      }
    }

    cv_.notify_all();

    /* Inform the client interested in 'init response', we are live */
    if (initResultListener) {
        initResultListener(serviceCurrentStatus_);
    }
}


/*
 * Audio server subsystem-restart process is either started or finished.
 * Update application about it.
 *
 * AudioManagerImpl::onSSRUpdate() and AudioManagerImpl::onServiceStatusUpdate()
 * are called from the same dispatcher thread therefore serialized. Therefore,
 * flag serviceCurrentStatus_ will have a valid value at any instant of time.
 */
void AudioManagerImpl::onSSRUpdate(telux::common::ServiceStatus newStatus) {
    {
      std::lock_guard<std::mutex> lock(serviceStatusGuard_);
      LOG(DEBUG, __FUNCTION__);

      if (AudioManagerImpl::exitNow_) {
          LOG(WARNING, __FUNCTION__, " dropping ssr update");
          return;
      }
      /*
       * Handle two or more consecutive service available or unavailable events.
       *
       * 1. SSR happens, service becomes unavailable, we sent unavailable
       *    status to the applcation.
       * 2. Application is now waiting for service available status.
       * 3. Server crashed, connection lost, AudioManagerImpl::onServiceStatusChange()
       *    invoked, leading to second consecutive service unavailable status message
       *    sent to application. Prevent sending this 2nd same status as there is no
       *    advantage of sending it to the application.
       */
      if (newStatus == serviceCurrentStatus_) {
          return;
      }
      serviceCurrentStatus_ = newStatus;
    }

    sendNewStatusToClients(newStatus);
}

/*
 * We are connected/disconnected from server. Update application about it.
 */
void AudioManagerImpl::onServiceStatusUpdate(telux::common::ServiceStatus newStatus) {
    {
      std::lock_guard<std::mutex> lock(serviceStatusGuard_);
      LOG(DEBUG, __FUNCTION__);

      if (AudioManagerImpl::exitNow_) {
          LOG(WARNING, __FUNCTION__, " dropping service status");
          return;
      }
      if (newStatus == serviceCurrentStatus_) {
          return;
      }
      serviceCurrentStatus_ = newStatus;
    }

    sendNewStatusToClients(newStatus);

    /* Re-subscribe for server-connection events */
    auto f = std::async(std::launch::async, [&]() {
                this->initSync(nullptr);
             }).share();

    asyncTaskQueue_.add(f);
}

/*
 * Update clients with the new service status.
 */
void AudioManagerImpl::sendNewStatusToClients(telux::common::ServiceStatus newStatus) {

    std::vector<std::weak_ptr<IAudioListener>> applisteners;

    if (newStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        /* Send new service status to all *StreamImpl object for internal state cleanup */
        for(auto &wp : createdStreams_){
            if (auto sp = wp.lock()) {
                sp->onServiceStatusChange();
            }
        }
        createdStreams_.clear();
    }

    /* Send new service status to all registered application listeners */
    if (!serviceStatusListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " invalid listener mgr");
        return;
    }
    serviceStatusListenerMgr_->getAvailableListeners(applisteners);

    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onServiceStatusChange(newStatus);
        }
    }

    if (newStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        /*
         * When application called an API and passed callback, its reference was cached
         * with CommandCallbackManager. We will never call this callback now, therefore
         * remove it.
         *
         * When service becomes unavailable, application should give up references to
         * all types of streams (*StreamImpl) as all of them have become invalid now.
         * Resources allocated to streams will be released in their destructors therefore
         * no explicit cleanup is required here.
         */
        cmdCallbackMgr_.reset();
    }

    LOG(DEBUG, __FUNCTION__, " new status sent ", static_cast<int>(newStatus));
}

/*
 * Gives current state of the audio service.
 */
telux::common::ServiceStatus AudioManagerImpl::getServiceStatus() {

    std::lock_guard<std::mutex> lock(serviceStatusGuard_);

    return serviceCurrentStatus_;
}

/*
 * Application registration for service status events.
 */
telux::common::Status AudioManagerImpl::registerListener(
        std::weak_ptr<IAudioListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return serviceStatusListenerMgr_->registerListener(listener);
}

/*
 * Application de-registration for service status events.
 */
telux::common::Status AudioManagerImpl::deRegisterListener(
        std::weak_ptr<IAudioListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return serviceStatusListenerMgr_->deRegisterListener(listener);
}

/*
 * Gives a list of currently supported audio device types like mic and speaker etc.
 */
telux::common::Status AudioManagerImpl::getDevices(GetDevicesResponseCb callback) {

    int cmdId;
    telux::common::Status status;

    /*
     * For all getFoo() APIs, ideally application should pass callback. However,
     * since it is an optional parameter and application may choose to not pass
     * it for any reason, we don't check for !callback case here. Requested info
     * is fetched from audio server and dropped at library level.
     */
    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    /*
     * CommandCallbackManager, internally saves memory address of the user provided
     * location as key and callback as the value in a map. Instead of passing memory
     * location, we are passing cmdId (integer) after typecasting it. Because integer
     * is 32 bit wide while pointer can be 32 or 64 bit wide, when typecasting back
     * from void * to integer, we get the actual value. Using this approach,
     * performance and memory gain is obtained by not allocating memory for every
     * API when sending async request.
     */
    status = transportClient_->getDevices(shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }
    return status;

}

void AudioManagerImpl::onGetDevicesResult(telux::common::ErrorCode ec,
        std::vector<DeviceType> deviceTypes,
        std::vector<DeviceDirection> deviceDirections, int cmdId) {

    uint32_t size = 0;
    std::shared_ptr<telux::audio::AudioDeviceImpl> device;
    std::shared_ptr<telux::common::ICommandCallback> resultListener;
    std::vector<std::shared_ptr<telux::audio::AudioDeviceImpl>> devices;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        /* Unexpected as we made supplying resultListener mandatory */
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    size = deviceTypes.size();

    for (uint32_t x = 0; x < size; ++x) {
        try {
            device = std::make_shared<telux::audio::AudioDeviceImpl>(
                        deviceTypes.at(x), deviceDirections.at(x));
        } catch (const std::exception& e) {
            LOG(ERROR, __FUNCTION__, " can't create AudioDeviceImpl");
            /* can't do anything, continue to help debugging */
            continue;
        }

        devices.emplace_back(device);
    }

    cmdCallbackMgr_.executeCallback(resultListener, devices, ec);
}

/*
 * Gives a list of currently supported audio stream types like playback and voice-call etc.
 */
telux::common::Status AudioManagerImpl::getStreamTypes(GetStreamTypesResponseCb callback) {

    int cmdId;
    telux::common::Status status;

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->getStreamTypes(shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onGetStreamsResult(telux::common::ErrorCode ec,
        std::vector<StreamType> streams, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, streams, ec);
}

/*
 * Applicable only for HAL, gives ACDB loaing and init status as obtained from HAL.
 */
telux::common::Status AudioManagerImpl::getCalibrationInitStatus(
        GetCalInitStatusResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    if (!callback) {
        return telux::common::Status::INVALIDPARAM;
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->getCalibrationInitStatus(shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onGetCalInitStatusResult(telux::common::ErrorCode ec,
        CalibrationInitStatus calibrationStatus, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, calibrationStatus, ec);
}

/*
 * Creates an audio stream with parameters specified by streamConfig. This method
 * causes stream creation on the server side whose ID is obtained in method
 * AudioManagerImpl::onCreateStreamResult().
 */
telux::common::Status AudioManagerImpl::createStream(StreamConfig streamConfig,
        CreateStreamResponseCb callback) {

    intptr_t cmdId;
    telux::common::Status status;

    if (!callback) {
        /*
         * When an audio stream has been created successfully on the server side,
         * corresponding *StreamImpl object is created on the client side to represent
         * this stream. Callback is the only way through which an application can
         * retrive this stream object and execute further operations on it. Therefore,
         * application must provide this callback.
         */
        return telux::common::Status::INVALIDPARAM;
    }

    if (streamConfig.deviceTypes.size() > MAX_DEVICES) {
        LOG(ERROR, __FUNCTION__, " exceeded maximum device count");
        return telux::common::Status::INVALIDPARAM;
    }

    if (streamConfig.voicePaths.size() > MAX_VOICE_PATH) {
        LOG(ERROR, __FUNCTION__, " exceeded maximum voice path count");
        return telux::common::Status::INVALIDPARAM;
    }

    cmdId = cmdCallbackMgr_.addCallback(callback);

    status = transportClient_->createStream(streamConfig, shared_from_this(), cmdId);

    if (status != telux::common::Status::SUCCESS) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

/*
 * Creates an audio stream with parameters specified by createdStreamInfo. This method
 * causes stream creation on the client side.
 *
 * If the stream creation is successfull on server-side but failure happens on client-side
 * delete the stream on server-side and return error to the application.
 */
void AudioManagerImpl::onCreateStreamResult(telux::common::ErrorCode ec,
        CreatedStreamInfo createdStreamInfo, int cmdId) {

    telux::common::Status status;
    std::shared_ptr<IAudioStream> audioStream;;
    std::shared_ptr<VoiceStreamImpl> voiceStream;
    std::shared_ptr<PlayStreamImpl> playStream;
    std::shared_ptr<CaptureStreamImpl> captureStream;
    std::shared_ptr<LoopbackStreamImpl> loopbackStream;
    std::shared_ptr<ToneGeneratorStreamImpl> toneStream;
    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        if (ec == telux::common::ErrorCode::SUCCESS) {
            ec = telux::common::ErrorCode::INVALID_ARGUMENTS;
            goto error2;
        } else {
            goto error1;
        }
    }

    if (ec != telux::common::ErrorCode::SUCCESS) {
        /* Stream creation failed on server-side itself, update application */
        cmdCallbackMgr_.executeCallback(resultListener, nullptr, ec);
        return;
    }

    /*
     * Stream created successfully on the server side, create corresponding *StreamImpl
     * proxy on the client side. Associate streamId with it to uniquely identify it.
     */
    try {
        switch(createdStreamInfo.streamType) {
            case StreamType::VOICE_CALL:
                voiceStream = std::make_shared<VoiceStreamImpl>(createdStreamInfo.streamId,
                            transportClient_);
                status = voiceStream->init();
                if (status != telux::common::Status::SUCCESS) {
                    ec = telux::common::ErrorCode::GENERIC_FAILURE;
                    goto error2;
                }
                audioStream = voiceStream;
                createdStreams_.push_back(voiceStream);
                break;
            case StreamType::PLAY:
                playStream = std::make_shared<PlayStreamImpl>(createdStreamInfo.streamId,
                            createdStreamInfo.writeMinSize,
                            createdStreamInfo.writeMaxSize, transportClient_);
                status = playStream->init();
                if (status != telux::common::Status::SUCCESS) {
                    ec = telux::common::ErrorCode::GENERIC_FAILURE;
                    goto error2;
                }
                audioStream = playStream;
                createdStreams_.push_back(playStream);
                break;
            case StreamType::CAPTURE:
                captureStream = std::make_shared<CaptureStreamImpl>(createdStreamInfo.streamId,
                            createdStreamInfo.readMinSize,
                            createdStreamInfo.readMaxSize, transportClient_);
                audioStream = captureStream;
                createdStreams_.push_back(captureStream);
                break;
            case StreamType::LOOPBACK:
                loopbackStream = std::make_shared<LoopbackStreamImpl>(createdStreamInfo.streamId,
                    transportClient_);
                audioStream = loopbackStream;
                createdStreams_.push_back(loopbackStream);
                break;
            case StreamType::TONE_GENERATOR:
                toneStream = std::make_shared<ToneGeneratorStreamImpl>(createdStreamInfo.streamId,
                    transportClient_);
                audioStream = toneStream;
                createdStreams_.push_back(toneStream);
                break;
            default:
                LOG(ERROR, __FUNCTION__, " invalid stream type ",
                    static_cast<int>(createdStreamInfo.streamType));
                ec = telux::common::ErrorCode::INVALID_ARGUMENTS;
                goto error2;
        }
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create *StreamImpl");
        ec = telux::common::ErrorCode::NO_MEMORY;
        goto error2;
    }

    /* Update application stream created successfully and pass reference to it */
    cmdCallbackMgr_.executeCallback(resultListener, audioStream, ec);
    return;

error2:
    /* Delete stream on the server side */
    transportClient_->deleteStream(createdStreamInfo.streamId, nullptr, 0);

error1:
    /* Update application stream creation failed */
    cmdCallbackMgr_.executeCallback(resultListener, nullptr, ec);
}

/*
 * Closes stream and release all resources allocated.
 */
telux::common::Status AudioManagerImpl::deleteStream(std::shared_ptr<IAudioStream> stream,
        DeleteStreamResponseCb callback) {

    intptr_t cmdId;
    uint32_t streamId;
    telux::common::Status status;
    std::shared_ptr<AudioStreamImpl> audioStreamImpl;

    if (!stream) {
        return telux::common::Status::INVALIDPARAM;
    }

    audioStreamImpl = std::dynamic_pointer_cast<AudioStreamImpl>(stream);

    streamId = audioStreamImpl->getStreamId();

    cmdId = INVALID_COMMAND_ID;
    if (callback) {
        cmdId = cmdCallbackMgr_.addCallback(callback);
    }

    status = transportClient_->deleteStream(streamId, shared_from_this(), cmdId);
    if (status != telux::common::Status::SUCCESS && callback) {
        cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    }

    return status;
}

void AudioManagerImpl::onDeleteStreamResult(telux::common::ErrorCode ec,
        uint32_t streamId, int cmdId) {

    std::shared_ptr<telux::common::ICommandCallback> resultListener;

    if ((cmdId) == INVALID_COMMAND_ID) {
        /* Caller not interested in knowing whether deleting stream
         * was successfull or failed */
        return;
    }

    resultListener = cmdCallbackMgr_.findAndRemoveCallback(cmdId);
    if (!resultListener) {
        LOG(ERROR, __FUNCTION__, " can't find callback, cmdId ", cmdId);
        return;
    }

    cmdCallbackMgr_.executeCallback(resultListener, ec);
}

/*
 * Creates two audio streams, playback and capture and configures them for transcoding.
 */
telux::common::Status AudioManagerImpl::createTranscoder(
        FormatInfo input, FormatInfo output, CreateTranscoderResponseCb callback) {
    return telux::common::Status::NOTSUPPORTED;
}

/* deprecated */
bool AudioManagerImpl::isSubsystemReady() {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    return (getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE);
}

bool AudioManagerImpl::waitForInitialization() {
    if (isSubsystemReady()) {
        return true;
    }

    std::unique_lock<std::mutex> cvLock(serviceStatusGuard_);
    cv_.wait(cvLock, [&] {
        return (serviceCurrentStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE);
    });

    return true;
}

/* deprecated */
std::future<bool> AudioManagerImpl::onSubsystemReady() {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    return std::async(std::launch::async, [&] { return waitForInitialization(); });
}

}  // End of namespace audio
}  // End of namespace telux
