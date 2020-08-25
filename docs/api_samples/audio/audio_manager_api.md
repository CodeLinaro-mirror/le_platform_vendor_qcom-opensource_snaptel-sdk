# Audio Manager API Sample Reference

This Section demonstrates how to use the Audio Manager API for audio subsystem/stream operations.

### 1. Get the AudioFactory and AudioManager instances

   ~~~~~~{.cpp}
   #include "AudioFactory.hpp"
   #include "AudioManager.hpp"

   using namespace telux::common;
   using namespace telux::audio;

   // Globals
   static std::shared_ptr<IAudioManager> audioManager;
   static std::shared_ptr<IAudioVoiceStream> audioVoiceStream;
   static unsigned int timeoutSec = 5;
   Status status;

   auto &audioFactory = audioFactory::getInstance();
   audioManager = audioFactory.getAudioManager();
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

Make sure that Audio subsystem is ready for services like voice call.
if subsystems were not ready, unconditionally wait or Timeout based wait.

   ~~~~~~{.cpp}
    std::future<bool> f = audioManager->onSubsystemReady();
    #if  //Timeout based wait
         if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
             std::cout << "operation timed out." << std::endl;
         } else {
             std::cout << "Audio Subsystem is ready." << std::endl;
         }
    #else //Unconditional wait
         bool subSystemsStatus = f.get();
         if (subSystemsStatus) {
           std::cout << "Audio Subsystem is ready." << std::endl;
         } else {
           std::cout << "Audio Subsystem is NOT ready." << std::endl;
         }
    #endif
   ~~~~~~

### 3. Query Supported Devices and Stream Types of Audio subsystem

Below methods provides details on supported Device Types and Stream Types

### 3.1 Query Supported Devices Types of Audio subsystem
   ~~~~~~{.cpp}
    //Callback to get supported device type details.
    void getDevicesCallback(std::vector<std::shared_ptr<IAudioDevice>> devices, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getDevices() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        int i = 0;
        for (auto device_type : devices) {
            std::cout << "Device [" << i << "] type: "
                << static_cast<unsigned int>(device_type->getType()) << ", direction: "
                << static_cast<unsigned int>(device_type->getDirection()) << std::endl;
            i++;
        }
    }

    //Query Supported Device type details.
    status = audioManager->getDevices(getDevicesCallback);
   ~~~~~~

### 3.2 Query Supported Stream Types of Audio subsystem
   ~~~~~~{.cpp}
    //Callback to get supported stream type details.
    void getStreamTypesCallback(std::vector<StreamType> streams, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getStreamTypes() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        int i = 0;
        for (auto stream_type : streams) {
            std::cout << "Stream [" << i << "] type: " << static_cast<unsigned int>(stream_type)
                << std::endl;
            i++;
        }
    }

    //Query Supported stream type details.
    status = audioManager->getStreamTypes(getStreamTypesCallback);
   ~~~~~~

### 4. Create an Audio Stream (Voice Call Session)
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

### 5. Delete an Audio Stream (Voice Call Session), which was created earlier
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
