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
struct PlaybackFile {

    /** Absolute path of the file */
    std::string absoluteFilePath;

    /** Defines how a file should be played */
    RepeatInfo repeatInfo;
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
     * @ref IAudioPlayer::stopPlayback() or when playback is stopped due to an error.
     *
     * This API will not be invoked if all the files in the playback list are
     * successfully played to completion. In such a scenario, @ref onPlaybackFinished
     * will be invoked on completion.
     */
    virtual void onPlaybackStopped() { }

    /**
     * Invoked whenever an error occurs.
     *
     * @param [in] error Appropriate error code @ref telux::common::ErrorCode
     *
     * @param [in] file File which was getting played when this error occurred.
     *             It can be empty if error occurred before opening any file.
     */
    virtual void onError(telux::common::ErrorCode error, std::string file) { }

    /**
     * Invoked whenever each file has been played from the playlist. If a file
     * is played repeatedly, it will be called that many times. If a file is
     * played indefinitely, it is called every time file is played completely.
     * If an error occurs or playback is stopped, this callback is not called.
     *
     * @param [in] file File played successfully
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
 * This class manages playback of a playlist of audio files. The playlist could contain
 * one or more files. Clients can also specify how many times the file should be played,
 * in case repetition is required.
 */
class IAudioPlayer {
 public:

   /**
    * Plays audio files as specified in the playlist. The playlist can contain one or
    * more files to play. How many times a file should be played can also be specified.
    * If a file is played indefinitely, playback can be stopped by calling stopPlayback()
    * at any time.
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_PLAY,
    * permission to invoke this method successfully.
    *
    * @param [in] streamConfig Audio stream parameters (format, sampling rate, devices etc.)
    *
    * @param [in] filesToPlay List of files to play. They all must be of same format;
    *             for example; all PCM or all AMRWB.
    *
    * @param [in] statusListener Receives various status updates in @ref IPlayListListener
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the playback is started
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
                  to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode startPlayback(
        StreamConfig streamConfig,
        std::vector<PlaybackFile> &filesToPlay,
        std::weak_ptr<IPlayListListener> statusListener) = 0;

   /**
    * Stops the playback started with @ref startPlayback().
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_PLAY,
    * permission to invoke this method successfully.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the playback is stopped.
    *          @ref telux::common::ErrorCode::INVALID_STATE, if there is no playback
    *          in-progress. An appropriate error code in all other cases.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
                  to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode stopPlayback() = 0;

   /**
    * Sets the volume level of the audio stream.
    *
    * Note - direction set in the StreamVolume is not used.
    *
    * @param [in] volume Specifies the volume level to set
    *
    * @returns ErrorCode @ref telux::common::ErrorCode::SUCCESS if the given volume is
    *           set successfully, otherwise, an appropriate error code.
    */
   virtual telux::common::ErrorCode setVolume(StreamVolume volume) = 0;

   /**
    * Retrieves the current volume level of the audio stream.
    *
    * @param [out] volume, Contains current volume information upon method return
    *
    * @returns ErrorCode @ref telux::common::ErrorCode::SUCCESS if the volume is retrieved
    *           successfully, otherwise, an appropriate error code.
    */
   virtual telux::common::ErrorCode getVolume(StreamVolume &volume) = 0;

    /**
     * Destructor of the IAudioPlayer.
     */
   virtual ~IAudioPlayer() { };
};

/** @} */ /* end_addtogroup telematics_audio_player */

}  // End of namespace audio
}  // End of namespace telux

#endif // TELUX_AUDIO_AUDIOPLAYER_HPP