# Audio Manager API Sample Reference for voice session volume/mute control

This Section demonstrates how to use the Audio Manager API for voice session volume/mute control.

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

### 5. Set volume on Started Audio Stream (Voice Call Session) for specified direction
   ~~~~~~{.cpp}
    //Callback which provides response to setVolume.
    void setStreamVolumeCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setVolume() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "setVolume() succeeded." << std::endl;
    }

    //Set volume on an Audio Stream (Voice Call Session) for RX direction
    StreamVolume streamVol;
    ChannelVolume channelVol;
    streamVol.dir = StreamDirection::RX;
    channelVol.channelType = ChannelType::LEFT;
    channelVol.vol = 0.5;
    streamVol.volume.emplace_back(channelVol);
    status = audioVoiceStream->setVolume(streamVol, setStreamVolumeCallback);
   ~~~~~~

### 6. Get volume on Started Audio Stream (Voice Call Session)
   ~~~~~~{.cpp}
    //Callback which provides response to getVolume.
    void getStreamVolumeCallback(StreamVolume volume, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getVolume() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "Volume direction: " << static_cast<uint32_t>(volume.dir) << std::endl;

        int i = 0;
        for (auto channel_volume : volume.volume) {
            std::cout << "ChannelVolume [" << i << "] channel type: "
                << static_cast<uint32_t>(channel_volume.channelType) << ", " << "volume: "
                << channel_volume.vol << std::endl;
        }
    }

    //Get volume on an Audio Stream (Voice Call Session) for RX direction
    status = audioVoiceStream->getVolume(StreamDirection::RX, getStreamVolumeCallback);
   ~~~~~~

### 7. Set Mute on Started Audio Stream (Voice Call Session) for specified direction
   ~~~~~~{.cpp}
    //Callback which provides response to setMute.
    void setStreamMuteCallback(ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "setMute() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "setMute() succeeded." << std::endl;
    }

    //Set Mute on an Audio Stream (Voice Call Session) for TX direction
    StreamMute mute;
    mute.dir = StreamDirection::TX;
    mute.enable = true; //true: enable, false: disable
    status = audioVoiceStream->setMute(mute, setStreamMuteCallback);
   ~~~~~~

### 8 Get Mute on Started Audio Stream (Voice Call Session) for specified direction
   ~~~~~~{.cpp}
    //Callback which provides response to getMute.
    void getStreamMuteCallback(StreamMute mute, ErrorCode error)
    {
        if (error != ErrorCode::SUCCESS) {
            std::cout << "getMute() returned with error " << static_cast<unsigned int>(error)
                << std::endl;
            return;
        }

        std::cout << "Mute enable: " << mute.enable << ", direction: "
            << static_cast<uint32_t>(mute.dir) << std::endl;

    }

    //Get Mute on an Audio Stream (Voice Call Session) for TX direction
    status = audioVoiceStream->getMute(StreamDirection::TX, getStreamMuteCallback);
   ~~~~~~

### 9. Stop Created Audio Stream (Voice Call Session)
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

### 10. Delete an Audio Stream (Voice Call Session), which was created earlier
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
