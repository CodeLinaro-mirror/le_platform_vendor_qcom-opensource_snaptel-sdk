/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AUDIOMANAGERIMPL_HPP
#define AUDIOMANAGERIMPL_HPP

#include <telux/audio/AudioListener.hpp>
#include <telux/audio/AudioManager.hpp>
#include "common/CommandCallbackManager.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/CommonUtils.hpp"
#include "AudioGrpcClientStub.hpp"
#include "ICommunicator.hpp"
#include "AudioStreamImpl.hpp"


namespace telux {
namespace audio {

class AudioManagerImpl : public IAudioManager,
                         public IGetStreamsCb,
                         public IGetCalInitStatusCb,
                         public IGetDevicesCb,
                         public ICreateStreamCb,
                         public IDeleteStreamCb,
                         public IServiceStatusEventsCb,
                         public std::enable_shared_from_this<AudioManagerImpl> {

 public:
    AudioManagerImpl();
    ~AudioManagerImpl();

    telux::common::Status init(telux::common::InitResponseCb initResultListener);

    /* Audio service availability management */

    void initSync(telux::common::InitResponseCb initResultListener);

    void onServiceStatusUpdate(telux::common::ServiceStatus newStatus) override;

    void onSSRUpdate(telux::common::ServiceStatus newStatus) override;

    /* IAudioManager overrides */

    telux::common::ServiceStatus getServiceStatus(void) override;

    telux::common::Status registerListener(std::weak_ptr<IAudioListener> listener) override;

    telux::common::Status deRegisterListener(std::weak_ptr<IAudioListener> listener) override;

    telux::common::Status getDevices(GetDevicesResponseCb callback = nullptr) override;

    telux::common::Status getStreamTypes(GetStreamTypesResponseCb callback = nullptr) override;

    telux::common::Status getCalibrationInitStatus(GetCalInitStatusResponseCb callback) override;

    telux::common::Status createStream(StreamConfig streamConfig,
        CreateStreamResponseCb callback = nullptr) override;

    telux::common::Status deleteStream(std::shared_ptr<IAudioStream> stream,
        DeleteStreamResponseCb callback = nullptr) override;

    telux::common::Status createTranscoder(
        FormatInfo input, FormatInfo output, CreateTranscoderResponseCb callback) override;

    /* IGetDevices overrides */
    void onGetDevicesResult(telux::common::ErrorCode ec,
        std::vector<DeviceType> deviceTypes,
        std::vector<DeviceDirection> deviceDirections, int cmdId) override;

    /* IGetStreams overrides */
    void onGetStreamsResult(telux::common::ErrorCode ec,
                    std::vector<StreamType> streams, int cmdId) override;

    /* IGetCalInitStatus overrides */
    void onGetCalInitStatusResult(telux::common::ErrorCode ec,
                    CalibrationInitStatus calibrationStatus, int cmdId) override;

    /* ICreateStream overrides */
    void onCreateStreamResult(telux::common::ErrorCode ec,
                    CreatedStreamInfo createdStreamInfo, int cmdId) override;

    /* IDeleteStream overrides */
    void onDeleteStreamResult(telux::common::ErrorCode ec,
                    uint32_t streamId, int cmdId) override;

    /* deprecated */
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

 private:
    static std::mutex serviceStatusGuard_;
    static std::atomic<bool> exitNow_;
    telux::common::CommandCallbackManager cmdCallbackMgr_;
    std::shared_ptr<ICommunicator> transportClient_;
    telux::common::AsyncTaskQueue<void> asyncTaskQueue_;
    std::shared_ptr<telux::common::ListenerManager<IAudioListener>> serviceStatusListenerMgr_;
    std::condition_variable cv_;
    std::vector<std::weak_ptr<AudioStreamImpl>> createdStreams_;

    telux::common::ServiceStatus serviceCurrentStatus_ =
            telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    bool waitForInitialization(void);
    void sendNewStatusToClients(telux::common::ServiceStatus newStatus);

    AudioManagerImpl(AudioManagerImpl const &) = delete;
    AudioManagerImpl &operator=(AudioManagerImpl const &) = delete;
};

}  // End of namespace audio
}  // End of namespace telux

#endif  // AUDIOMANAGERIMPL_HPP
