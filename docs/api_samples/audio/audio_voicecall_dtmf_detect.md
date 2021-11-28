Detect DTMF tones {#audio_voicecall_dtmf_detect}
=============================================================================================

## Using Audio Manager APIs to detect DTMF tones in a voice call

This section demostrates how to use audio APIs to detect DTMF tones during an active voice call.
Note that only Rx direction is supported currently.

### 1. Get the AudioFactory instance

   ~~~~~~{.cpp}
    auto &audioFactory = AudioFactory::getInstance();
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

### 3. Exit the application, if SDK is unable to initialize Audio subsystem

   ~~~~~~{.cpp}
    if(isReady) {
        std::cout << " *** Audio subsystem is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize Audio subsystem " << std::endl;
        return 1;
    }
   ~~~~~~

### 4. Create an audio Stream (to be associated with Voice call session)

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

### 5. Start the Voice call session

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

### 6. Register a listener to get notifications on DTMF tone detection

   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void registerListenerCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "registerListener() failed with error " << static_cast<int>(error)
                                << std::endl;
            return;
        }
        std::cout << "registerListener() succeeded." << std::endl;
    }
    // Instantiate MyVoiceListener
    auto myDtmfToneListener = std::make_shared<MyVoiceListener>();
    // Register the listener
    status = audioVoiceStream->registerListener(myDtmfToneListener, registerListenerCallback);
   ~~~~~~

### 7. De-register the listener to stop getting the notifications

   ~~~~~~{.cpp}
    status = audioVoiceStream->deRegisterListener(myDtmfToneListener);
   ~~~~~~

### 8. Stop the Voice call session

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

### 9. Delete the audio stream associated with the Voice call session

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
