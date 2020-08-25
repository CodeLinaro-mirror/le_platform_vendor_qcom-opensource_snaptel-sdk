# Audio Manager API Sample Reference for audio loopback session.

Please follow the below steps to start/stop loopback on a loopback session.

### 1. Get the Audio Factory and Audio Manager instances ###
   ~~~~~~{.cpp}
    auto &audioFactory = AudioFactory::getInstance();
    auto audioManager = audioFactory.getAudioManager();
   ~~~~~~

### 2. Wait for the Audio subsystem to be initialized and ready ###
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

### 3. Exit the application, if SDK is unable to initialize Audio subsystem ###
   ~~~~~~{.cpp}
    if (isReady) {
        std::cout << " *** Audio subsystem is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize Audio subsystem " << std::endl;
        return 1;
    }
   ~~~~~~

### 4. Create an audio Stream (to be associated with loopback)  ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioLoopbackStream = std::dynamic_pointer_cast<IAudioLoopbackStream>(stream);
    }
    // Create a loopback stream with required configuration
    config.type = telux::audio::StreamType::LOOPBACK;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);
    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 6. Start loopback between the specified source and sink devices ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void startLoopbackCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "startLoopback() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "startLoopback() succeeded." << std::endl;
    }
    // start loopback
    status = audioLoopbackStream->startLoopback(startLoopbackCallback);
   ~~~~~~

### 7. Stop loopback between the specified source and sink devices ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void stopLoopbackCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopLoopback() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "stopLoopback() succeeded." << std::endl;
    }
    // Stop the loopback, which was started earlier
    status = audioLoopbackStream->stopLoopback(stopLoopbackCallback);
   ~~~~~~

### 8. Delete the audio stream associated with the Loopback session ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioLoopbackStream.reset();
    }
    // Delete the Audio Stream
    status = audioManager->deleteStream(std::dynamic_pointer_cast<IAudioStream>(audioLoopbackStream),
                                    deleteStreamCallback);
   ~~~~~~
