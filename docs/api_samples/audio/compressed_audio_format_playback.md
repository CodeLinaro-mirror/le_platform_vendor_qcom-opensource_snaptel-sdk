# Audio Manager APIs Sample Reference for compressed audio format playback

This Section demonstrates how to use the Audio Manager API for compressed audio format playback.

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
    #if  // Timeout based wait
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "operation timed out." << std::endl;
        } else {
            subSystemsStatus = f.get();
            if (subSystemsStatus) {
                std::cout << "Audio Subsystem is ready." << std::endl;
            }
        }
    #else // Unconditional wait
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
    // Callback which provides response to createStream with pointer to base interface IAudioStream.
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
    // Create an Audio Stream (Audio Playback Session)
    StreamConfig config;
    config.type = StreamType::PLAY;
    config.sampleRate = 48000;
    config.format = AudioFormat::AMRWB_PLUS;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    AmrwbpParams params;
    params.bitWidth = 16;
    params.frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
    config.formatParams = &params;
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
    // Callback which provides response to write operation.
    void writeCallback(std::shared_ptr<IStreamBuffer> buffer, uint32_t bytes, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "write() returned with error " << static_cast<int>(error) << std::endl;
            // Application needs to resend the Bitstream buffer from leftover position if bytes
            // consumed are not equal to requested number of bytes to be written.
            pipeLineEmpty_ = false;
        }
        buffer->reset();
        return;
    }

    // Indiction Received only when callback returns with error that bytes written are not equal to
    // bytes requested to write. It notifies that pipeline is ready to accept new buffer to write.
    void onReadyForWrite() {
        pipeLineEmpty_ = true;
    }

    // Write desired data into the buffer, the bytes sent as 0x1 for example purpose only.
    // First write starts Playback Session.
    memset(streamBuffer->getRawBuffer(),0x1,size);
    auto status = audioPlayStream->write(streamBuffer, writeCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "write() failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to write to stream sent" << std::endl;
    }
   ~~~~~~
### 6.1 Stop playback operation(STOP_AFTER_PLAY : Stops after playing pending buffers in pipeline)
   ~~~~~~
    std::promise<bool> p;
    auto status = audioPlayStream_->stopAudio(StopType::STOP_AFTER_PLAY, [&p](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to stop after playing buffers" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to stop playback after pending buffers Sent" << std::endl;
    } else {
        std::cout << "Request to stop playback after pending buffers failed" << std::endl;
    }
    if (p.get_future().get()) {
        std::cout << "Pending buffers played successful !!" << std::endl;
    }
   ~~~~~~

### 6.2 Stop playback operation(FORCE_STOP : Stops immediately, all buffers in pipeline are flushed)
   ~~~~~~
    std::promise<bool> p;
        auto status = audioPlayStream_->stopAudio(
            StopType::FORCE_STOP, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to force stop" << std::endl;
            }
            });
        if(status == telux::common::Status::SUCCESS){
            std::cout << "Request to force stop Sent" << std::endl;
        } else {
            std::cout << "Request to force stop failed" << std::endl;
        }
        if (p.get_future().get()) {
                std::cout << "Force Stop successful !!" << std::endl;
        }
   ~~~~~~

### 7. Delete an Audio Stream (Audio Playback Session), once reached end of operation.
   ~~~~~~{.cpp}
    // Callback which provides response to deleteStream
    void deleteStreamCallback(ErrorCode error) {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "deleteStream() returned with error " << static_cast<int>(error)
                << std::endl;
            return;
        }
        std::cout << "deleteStream() succeeded." << std::endl;
        audioPlayStream = nullptr;
    }
    // Delete an Audio Stream (Audio Playback Session), once reached end of operation.
    Status  status = audioManager->deleteStream(audioPlayStream, deleteStreamCallback);
    if (status != Status::SUCCESS) {
        std::cout << "deleteStream failed with error" << static_cast<int>(status) << std::endl;
    }
   ~~~~~~
