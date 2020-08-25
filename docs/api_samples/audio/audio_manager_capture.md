# Audio Manager API Sample Reference for audio capture session

This Section demonstrates how to use the Audio Manager API for audio capture session.

### 1. Get the AudioFactory and AudioManager instances

   ~~~~~~{.cpp}
    auto &audioFactory = audioFactory::getInstance();
    auto audioManager = audioFactory.getAudioManager();
   ~~~~~~

### 2. Check if Audio subsystem is ready

   ~~~~~~{.cpp}
    if (audioManager) {
        bool subSystemsStatus = audioManager->isSubsystemReady();
        if (subSystemsStatus) {
            std::cout << "Audio Subsystem is ready." << std::endl;
        } else {
            std::cout << "Audio Subsystem is NOT ready." << std::endl;
        }
    } else {
        std::cout << "Invalid Audio manager" << std::endl;
    }
   ~~~~~~

### 2.1 If Audio subsystem is not ready, wait for it to be ready

Make sure that Audio subsystem is ready for services like audio capture.
If subsystem is not ready, wait unconditionally (or) until a timeout.

   ~~~~~~{.cpp}
    std::future<bool> f = audioManager->onSubsystemReady();
    #if  //Timeout based wait
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "operation timed out." << std::endl;
        } else {
            subSystemsStatus = f.get();
            if (subSystemsStatus) {
                std::cout << "Audio Subsystem is ready." << std::endl;
            }
        }
    #else //Unconditional wait
        subSystemsStatus = f.get();
        if (subSystemsStatus) {
            std::cout << "Audio Subsystem is ready." << std::endl;
        } else {
            std::cout << "Audio Subsystem is NOT ready." << std::endl;
        }
    #endif
   ~~~~~~

### 3. Create an Audio Stream (Audio Capture Session)
   ~~~~~~{.cpp}
    //Callback which provides response to createStream, with pointer to base interface IAudioStream.
    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "createStream() returned with error " << static_cast<int>(error)
                << std::endl;
            return;
        }
        std::cout << "createStream() succeeded." << std::endl;
        audioCaptureStream = std::dynamic_pointer_cast<IAudioCaptureStream>(stream);
    }

    //Create an Audio Stream (Audio Capture Session)
    StreamConfig config;
    config.type = StreamType::CAPTURE;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Allocate Stream Buffers for Capture Operation
   ~~~~~~{.cpp}
    // Get an audio buffer (can get more than one)
    auto streamBuffer = audioCaptureStream->getStreamBuffer();
    if(streamBuffer != nullptr) {
        // Setting the bytesToRead (bytes to be read from stream) as minimum size
        // required by stream. In any case if size returned is 0, using the Maximum Buffer
        // Size, any buffer size between min and max can be used
        bytesToRead = streamBuffer->getMinSize();
        if(bytesToRead == 0) {
            bytesToRead = streamBuffer->getMaxSize();
        }
    } else {
        std::cout << "Failed to get Stream Buffer " << std::endl;
        return EXIT_FAILURE;
    }
   ~~~~~~

### 5. Start read operation for the capture to Start
   ~~~~~~{.cpp}
    //Callback which provides response to read operation
    void readCallback(std::shared_ptr<IStreamBuffer> buffer, ErrorCode error)
    {
        uint32_t bytesWrittenToFile = 0;
        if (error != ErrorCode::SUCCESS) {
            std::cout << "read() returned with error " << static_cast<int>(error) << std::endl;
        } else {
            uint32_t size = buffer->getDataSize();
            std::cout << "Successfully read " << size << " bytes" << std::endl;
        }
        buffer->reset();
        return;
    }
    //Read from Capture
    //First read starts Capture Session.
    auto status = audioCaptureStream->read(streamBuffer, bytesToRead, readCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "read() failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to read stream sent" << std::endl;
    }
   ~~~~~~
### 6. Delete an Audio Stream (Audio Capture Session), once required bytes captured.
   ~~~~~~{.cpp}
    //Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<int>(error)
                        << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioCaptureStream.reset();
    }
    //Delete an Audio Stream (Audio Capture Session), once reached end of operation.
    Status  status = audioManager->deleteStream(
               std::dynamic_pointer_cast<IAudioStream>(audioCaptureStream), deleteStreamCallback);
    if (status != Status::SUCCESS) {
        std::cout << "deleteStream failed with error" << static_cast<int>(status) << std::endl;
    }
   ~~~~~~
