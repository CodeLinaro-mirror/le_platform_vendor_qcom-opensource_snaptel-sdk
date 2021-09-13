Using Audio Manager APIs to play DTMF tone in a voice call. {#audio_voicecall_dtmf_play}
========================================================================================

# Using Audio Manager APIs to play DTMF tone in a voice call

Please follow the below steps to play a DTMF tone in an active voice call. Note that only Rx direction is supported now.

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

### 3. Create an audio Stream (to be associated with Voice call session)

   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioVoiceStream = std::dynamic_pointer_cast<IAudioVoiceStream>(stream);
    }
    // Create a voice stream with required configuration
    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Start the Voice call session

   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void startAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "startAudio() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "startAudio() succeeded." << std::endl;
    }
    // Start the Voice call session
    status = audioVoiceStream->startAudio(startAudioCallback);
   ~~~~~~

### 5. Play a DTMF tone

   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void playDtmfCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "playDtmfTone() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "playDtmfTone() succeeded." << std::endl;
    }
    // Play the DTMF tone with required configuration
    status = audioVoiceStream->playDtmfTone(dtmfTone, duration, gain, playDtmfCallback);
   ~~~~~~

### 6. Stop the Voice call session

   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void stopAudioCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopAudio() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "stopAudio() succeeded." << std::endl;
    }
    // Stop the Voice call session, which was started earlier
    status = audioVoiceStream->stopAudio(stopAudioCallback);
   ~~~~~~

### 7. Delete the audio stream associated with the Voice call session

   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioVoiceStream->reset();
    }
    //Delete the Audio Stream
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioVoiceStream),
                                    deleteStreamCallback);
   ~~~~~~
