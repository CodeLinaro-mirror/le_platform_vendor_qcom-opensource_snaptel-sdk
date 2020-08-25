# Audio Manager API Sample Reference for voice session device switch

This Section demonstrates how to use the Audio Manager API for voice session device switch.

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
### 5. Device switch on Started Audio Stream (Voice Call Session)
   ~~~~~~{.cpp}
    //Callback which provides response to setDevice.
    void setStreamDeviceCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setDevice() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "setDevice() succeeded." << std::endl;
    }

    //Set New Device for an Audio Stream (Voice Call Session)
    std::vector<DeviceType> devices;
    devices.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER); //Set new device type
    status = audioVoiceStream->setDevice(devices, setStreamDeviceCallback);
   ~~~~~~

### 6. Query Device details on Started Audio Stream (Voice Call Session)
   ~~~~~~{.cpp}
    //Callback which provides response to getDevice.
    void getStreamDeviceCallback(std::vector<DeviceType> devices, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getDevice() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        int i = 0;
        for (auto device_type : devices) {
            std::cout << "Device [" << i << "] type: " << static_cast<uint32_t>(device_type)
                << std::endl;
            i++;
        }
    }

    //get Device details of an Audio Stream (Voice Call Session)
    status = audioVoiceStream->getDevice(getStreamDeviceCallback);
   ~~~~~~

### 7. Stop Created Audio Stream (Voice Call Session)
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

### 8. Delete an Audio Stream (Voice Call Session), which was created earlier
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
