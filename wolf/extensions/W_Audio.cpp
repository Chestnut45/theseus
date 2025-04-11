//-----------------------------------------------------------------------------
// File:			W_Audio.cpp
// Original Author:	D'Anyil Landry
//
// A static class providing simple access to audio playback for applications.
//-----------------------------------------------------------------------------

#include "W_Audio.h"

namespace wolf
{

void Audio::Play(const std::string& filepath, float volume, float pitchOffset, float pan, bool falloff, bool loop, float loopPoint)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = s_samples[filepath];

    // Load from file on first play
    if (sound.getLength() == 0) sound.load(filepath.c_str());

    // Setup loop state before playing
    sound.setLooping(loop);
    sound.setLoopPoint(loopPoint);

    // Play the sound with the given arguments
    auto handle = s_core.play(sound);
    float adjustedVolume = volume;

    // Calculate falloff if requested
    if (falloff)
    {
        // Ensure valid handle map state
        auto& handles = s_handles[filepath];
        
        // Count active voices playing this sound
        int activeVoices = 1;
        for (int i = 0; i < handles.size(); ++i)
        {
            // Remove invalid handles
            if (!s_core.isValidVoiceHandle(handles[i]))
            {
                handles.erase(handles.begin() + i);
                i--;
                continue;
            }

            // Increase handle count
            activeVoices++;
        }
        handles.push_back(handle);

        // Adjust volume with inverse falloff based on number of active voices
        adjustedVolume = volume / activeVoices;
    }
    
    s_core.setVolume(handle, adjustedVolume);
    s_core.setPan(handle, pan);

    // Protect background music from being killed, but allow regular sfx to be killed in case of overload
    if (filepath.starts_with("data/sounds/bgm_")) s_core.setProtectVoice(handle, true);

    // Dirty awful hack pitch shifting (barf)
    // TODO: Literally anything other than this
    if (pitchOffset != 0.0f) s_core.setSamplerate(handle, s_core.getSamplerate(handle) + pitchOffset);

}

void Audio::Stop(const std::string& filepath)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = s_samples[filepath];

    // Stop all instances of the sample
    s_core.stopAudioSource(sound);
}

void Audio::Stop()
{
    s_core.stopAll();
}

void Audio::Load(const std::string& filepath)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = s_samples[filepath];

    // Ensure no instances are playing this sound
    // Loading during playback can crash
    s_core.stopAudioSource(sound);

    sound.load(filepath.c_str());
}

void Audio::_Setup()
{
    s_core.init(1U, SoLoud::Soloud::BACKENDS::MINIAUDIO);
}

void Audio::_Shutdown()
{
    s_core.stopAll();
    s_core.deinit();
}

}