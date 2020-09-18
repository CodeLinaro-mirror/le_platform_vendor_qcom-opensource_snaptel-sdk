# Using Audio Manager APIs to detect DTMF tones in a voice call.

Please follow the below steps to detect DTMF tones in an active voice call. Note that only Rx direction is supported now.

### 1. Implement IVoiceListener interface ###
   ~~~~~~{.cpp}
    class MyVoiceListener : public IVoiceListener {
    public:
        void onDtmfToneDetection(DtmfTone dtmfTone) override;
    };
   ~~~~~~

### 2. Get the Audio Factory and Audio Manager instances ###
   ~~~~~~{.cpp}
    auto &audioFactory = AudioFactory::getInstance();
    auto audioManager = audioFactory.getAudioManager();
   ~~~~~~

### 3. Wait for the Audio subsystem to be initialized and ready ###
   ~~~~~~{.cpp}
    if (audioManager) {
        bool isReady = audioManager->isSubsystemReady();
        if (!isReady) {
            std::cout << "Audio subsystem is not ready, waiting for it to be ready " << std::endl;
            std::future<bool> f = audioManager->onSubsystemReady();
            isReady = f.get();
        }
    } else {
        std::cout << "Invalid Audio manager" << std::endl;
    }
   ~~~~~~

### 4. Exit the application, if SDK is unable to initialize Audio subsystem ###
   ~~~~~~{.cpp}
    if(isReady) {
        std::cout << " *** Audio subsystem is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize Audio subsystem " << std::endl;
        return 1;
    }
   ~~~~~~

### 5. Create an audio Stream (to be associated with Voice call session)  ###
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

### 6. Start the Voice call session ###
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

### 7. Register a listener to get notifications on DTMF tone detection ###
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

### 8. De-register the listener to stop getting the notifications ###
   ~~~~~~{.cpp}
    status = audioVoiceStream->deRegisterListener(myDtmfToneListener);
   ~~~~~~

### 9. Stop the Voice call session ###
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

### 10. Delete the audio stream associated with the Voice call session ###
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
