# Audio Manager API Sample Reference for audio tone generation.

Please follow the below steps to play a tone in an active tone generator stream.

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
    if(isReady) {
        std::cout << " *** Audio subsystem is Ready *** " << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize Audio subsystem " << std::endl;
        return 1;
    }
   ~~~~~~

### 4. Create an audio Stream (to be associated with tone generator)  ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioToneGeneratorStream = std::dynamic_pointer_cast<IAudioToneGeneratorStream>(stream);
    }
    // Create a tone generator stream with required configuration
    config.type = telux::audio::StreamType::TONE_GENERATOR;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 6. Play tone on a sink device ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void playToneCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "playTone() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "playTone() succeeded." << std::endl;
    }
    // Play the tone with required configuration
    status = audioToneGeneratorStream->playTone(freq, duration, gain, playToneCallback);
   ~~~~~~

### 7. Optionally, you can stop the tone being played before the specified duration elapses ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void stopToneCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "stopTone() failed with error " << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "stopTone() succeeded." << std::endl;
    }
    // Stop the tone play, which was started earlier
    status = audioToneGeneratorStream->stopTone(stopToneCallback);
   ~~~~~~

### 8. Delete the audio stream associated with the Tone Generator session ###
   ~~~~~~{.cpp}
    // Implement a response function to get the request status
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() failed with error" << static_cast<int>(error) << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioToneGeneratorStream.reset();
    }
    //Delete the Audio Stream
    status = audioManager->deleteStream(
        std::dynamic_pointer_cast<IAudioStream>(audioToneGeneratorStream), deleteStreamCallback);
   ~~~~~~
