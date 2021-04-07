Audio voice session start and stop {#audio_manager_voicecall_start_stop}
========================================================================

# Audio Manager API Sample Reference for voice session start and stop

This Section demonstrates how to use the Audio Manager API for voice session start and stop.

### 1. Get the AudioFactory instance

   ~~~~~~{.cpp}
    auto &audioFactory = audioFactory::getInstance();
   ~~~~~~

### 2. Get the AudioManager object and check for audio subsystem Readiness

   ~~~~~~{.cpp}
    std::promise<ServiceStatus> prom{};
    //  Get AudioManager instance.
    audioManager = audioFactory.getAudioManager([&prom](ServiceStatus serviceStatus) {
        prom.set_value(serviceStatus);
    });
    if (!audioManager) {
        std::cout << "Failed to get AudioManager object" << std::endl;
        return;
    }

    //  Check if audio subsystem is ready
    //  If audio subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = audioManager->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    //  Check the service status again.
    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsytem is Ready << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }
   ~~~~~~

### 3. Create an Audio Stream (Voice Call Session)

   ~~~~~~{.cpp}
    //Callback which provides response to createStream, with pointer to base interface IAudioStream.
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "createStream() succeeded." << std::endl;
        audioVoiceStream = std::dynamic_pointer_cast<IAudioVoiceStream>(stream);
    }

    //Create an Audio Stream (Voice Call Session)
    StreamConfig config;
    config.type = StreamType::VOICE_CALL;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = 16000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Start Created Audio Stream (Voice Call Session)

   ~~~~~~{.cpp}
    //Callback which provides response to startAudio.
    void startAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "startAudio() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "startAudio() succeeded." << std::endl;
    }

    //Start an Audio Stream (Voice Call Session)
    status = audioVoiceStream->startAudio(startAudioCallback);
   ~~~~~~

### 5. Stop Created Audio Stream (Voice Call Session)

   ~~~~~~{.cpp}

    //Callback which provides response to stopAudio.
    void stopAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopAudio() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "stopAudio() succeeded." << std::endl;
    }

    //Stop an Audio Stream (Voice Call Session), which was started earlier
    status = audioVoiceStream->stopAudio(stopAudioCallback);
   ~~~~~~

### 6. Delete an Audio Stream (Voice Call Session), which was created earlier

   ~~~~~~{.cpp}
    //Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "deleteStream() succeeded." << std::endl;
        audioVoiceStream.reset();
    }
    //Delete an Audio Stream (Voice Call Session), which was created earlier
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                        deleteStreamCallback);
   ~~~~~~
