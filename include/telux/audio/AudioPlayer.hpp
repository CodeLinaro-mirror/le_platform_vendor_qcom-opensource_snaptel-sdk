/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file  AudioPlayer.hpp
 * @brief This class manages playback of a playlist of audio files.
 *        The playlist could contain one or more files. Clients can
 *        also specify how many times the file should be played, in
 *        case repetition is required.
 */

#ifndef TELUX_AUDIO_AUDIOPLAYER_HPP
#define TELUX_AUDIO_AUDIOPLAYER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/audio/AudioDefines.hpp>

namespace telux {
namespace audio {

/** @addtogroup telematics_audio_player
 * @{ */

/**
 *  Defines whether to play or skip this file.
 */
enum class RepeatType {

    /** Play the file for given number of times */
    COUNT,

    /** Play the file indefinitely */
    INDEFINITELY,

    /** Do not play the file */
    SKIP
};

/**
 *  Defines number of times a file should be played.
 */
struct RepeatInfo {

    /** Please refer @ref RepeatType for details */
    RepeatType type;

    /** When using @ref RepeatType::COUNT,
     *  defines number of times a file should be played */
    uint32_t count;
};

/**
 *  Specifies files to play and how to play.
 */
struct PlaybackConfig {

    /** Absolute path of the file */
    std::string absoluteFilePath;

    /** Defines how a file should be played */
    RepeatInfo repeatInfo;

    /** Defines how an audio stream should be configured to play this file */
    StreamConfig streamConfig;
};

/**
 *  Receives status of the playback.
 */
class IPlayListListener {
 public:

    /**
     * Invoked when playback is started as a response to explicitly calling
     * @ref IAudioPlayer::startPlayback().
     */
    virtual void onPlaybackStarted() { }

    /**
     * Invoked whenever playback is stopped as a response to explicitly calling
     * @ref IAudioPlayer::stopPlayback() or when playback is stopped due to an error
     * for example audio can not be played or volume/mute can not be set.
     *
     * This API will not be invoked if all the files in the playback list are
     * successfully played to completion. In such a scenario, @ref onPlaybackFinished
     * will be invoked on completion.
     */
    virtual void onPlaybackStopped() { }

    /**
     * Invoked whenever an error occurs.
     *
     * @param[in] error Appropriate error code @ref telux::common::ErrorCode
     *
     * @param[in] file File which was getting played when this error occurred.
     *            It can be empty if an error occurred before opening any file.
     */
    virtual void onError(telux::common::ErrorCode error, std::string file) { }

    /**
     * Invoked whenever each file has been played from the playlist. If a file
     * is played repeatedly, it will be called that many times. If a file is
     * played indefinitely, it is called every time file is played completely.
     * If an error occurs or playback is stopped, this callback is not called.
     *
     * @param[in] file File played successfully
     */
    virtual void onFilePlayed(std::string file) { }

    /**
     * Invoked to confirm all files have been played gracefully as specified by
     * @ref IAudioPlayer::startPlayback().
     */
    virtual void onPlaybackFinished() { }

    /**
     * Destructor of IPlayListListener.
     */
    virtual ~IPlayListListener() { }
};

/**
 * IAudioPlayer manages playback of a playlist of audio files. The playlist could contain
 * one or more files. Clients can also specify how many times the file should be played,
 * in case repetition is required.
 */
class IAudioPlayer {
 public:
   /**
    * Plays audio files in the manner as specified by the `playbackConfigs`. Files can
    * have same or different audio format. Multiple files can be specified in one call
    * to this method.
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_PLAY,
    * permission to invoke this method successfully.
    *
    * @param[in] playbackConfigs List of files to play and the manner in which to play
    *
    * @param[in] statusListener Receives various status updates in @ref IPlayListListener
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the playback is started,
    *          @ref telux::common::ErrorCode::ACCESS_DENIED, if the caller doesn't have
    *          permission to call this API, @ref telux::common::ErrorCode::INVALID_STATE,
    *          if a playback is already in progress,
    *          @ref telux::common::ErrorCode::INVALID_ARGUMENTS, if no files are given,
    *          an appropriate error code in all other cases.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode startPlayback(
        std::vector<PlaybackConfig> &playbackConfigs,
        std::weak_ptr<IPlayListListener> statusListener) = 0;

   /**
    * Stops the playback started with @ref startPlayback().
    *
    * If the file is configured with RepeatType::COUNT, there is no need to call this
    * method as playback will stop automatically after file has been played successfully.
    * However, client can call this method anytime to stop playback explicitly.
    *
    * If the file is configured with RepeatType::INDEFINITELY, calling this method is
    * the only way to stop playback.
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_PLAY,
    * permission to invoke this method successfully.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the playback is stopped,
    *          @ref telux::common::ErrorCode::INVALID_STATE, if there is no playback
    *          in-progress, @ref telux::common::ErrorCode::ACCESS_DENIED, if the caller
    *          doesn't have permission to call this API, an appropriate error code in
    *          all other cases.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode stopPlayback() = 0;

   /**
    * Sets the volume level of the audio stream.
    *
    * Note - direction set in the StreamVolume is not used.
    *
    * @param[in] volume Specifies the volume level to set
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the given volume is set,
    *          otherwise, an appropriate error code.
    */
   virtual telux::common::ErrorCode setVolume(StreamVolume volume) = 0;

   /**
    * Retrieves the current volume level of the audio stream.
    *
    * @param[out] volume, Current volume
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the volume is retrieved,
    *          otherwise, an appropriate error code.
    */
   virtual telux::common::ErrorCode getVolume(StreamVolume &volume) = 0;

   /**
    * Mutes or unmutes the audio.
    *
    * @param[in] enable True to mute the audio, false to unmute the audio
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the playback is muted/unmuted
    *          as specified otherwise, an appropriate error code.
    */
   virtual telux::common::ErrorCode setMute(bool enable) = 0;

   /**
    * Retrieves the current mute state of the audio stream.
    *
    * @param[out] enable True if the audio is muted currently otherwise false
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the mute state is
    *          retrieved, otherwise, an appropriate error code.
    */
   virtual telux::common::ErrorCode getMute(bool &enable) = 0;

    /**
     * Destructor of the IAudioPlayer.
     */
   virtual ~IAudioPlayer() { };
};

/** @} */ /* end_addtogroup telematics_audio_player */

}  // End of namespace audio
}  // End of namespace telux

#endif // TELUX_AUDIO_AUDIOPLAYER_HPP