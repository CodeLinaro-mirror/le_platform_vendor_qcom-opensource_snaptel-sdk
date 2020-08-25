# Audio Manager API Sample Reference for audio playback session

This Section demonstrates how to use the Audio Manager API for audio playback session.

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

Make sure that Audio subsystem is ready for services like audio play.
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

### 3. Create an Audio Stream (Audio Playback Session)
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
        audioPlayStream = std::dynamic_pointer_cast<IAudioPlayStream>(stream);
    }
    //Create an Audio Stream (Audio Playback Session)
    StreamConfig config;
    config.type = StreamType::PLAY;
    config.sampleRate = 48000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    status = audioManager->createStream(config, createStreamCallback);
   ~~~~~~

### 4. Allocate Stream buffers for Playback operation
   ~~~~~~{.cpp}
    // Get an audio buffer (can get more than one)
    auto streamBuffer = audioPlayStream->getStreamBuffer();
    if (streamBuffer != nullptr) {
        // Setting the size that is to be written to stream as the minimum size
        // required by stream. In any case if size returned is 0, using the Maximum
        // Buffer Size, any buffer size between min and max can be used
        size = streamBuffer->getMinSize();
        if (size == 0) {
            size =  streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(size);
    } else {
        std::cout << "Failed to get Stream Buffer " << std::endl;
    }
   ~~~~~~

### 5. Start write operation for playback to start
   ~~~~~~{.cpp}
    //Callback which provides response to write operation.
    void writeCallback(std::shared_ptr<IStreamBuffer> buffer, uint32_t size, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "write() returned with error " << static_cast<int>(error) << std::endl;
        } else {
            std::cout << "Successfully written " << size << " bytes" << std::endl;
        }
        buffer->reset();
        return;
    }
    //Write desired data into the buffer
    //First write starts Playback Session.
    memset(streamBuffer->getRawBuffer(),0x1,size);
    auto status = audioPlayStream->write(streamBuffer, writeCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "write() failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to write to stream sent" << std::endl;
    }
   ~~~~~~
### 6. Delete an Audio Stream (Audio Playback Session), once reached end of operation.
   ~~~~~~{.cpp}
    //Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<int>(error)
                << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioPlayStream.reset();
    }
    //Delete an Audio Stream (Audio Playback Session), once reached end of operation.
    Status  status = audioManager->deleteStream(
               std::dynamic_pointer_cast<IAudioStream>(audioPlayStream), deleteStreamCallback);
    if (status != Status::SUCCESS) {
        std::cout << "deleteStream failed with error" << static_cast<int>(status) << std::endl;
    }
   ~~~~~~
