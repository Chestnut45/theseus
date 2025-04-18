//-----------------------------------------------------------------------------
// File:			W_Audio.cpp
// Original Author:	D'Anyil Landry
//
// A static class providing simple access to audio playback for applications.
//-----------------------------------------------------------------------------

#include "W_Audio.h"

#include <glm/glm.hpp>

namespace wolf
{

void Audio::Play(const std::string& filepath, float volume, float pitchOffset, float pan, bool falloff, bool loop, float loopPoint)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = s_samples[filepath];

    // Load from file on first play
    if (sound.getLength() == 0) sound.load(filepath.c_str());

    // Skip playing if within cooldown window
    auto& handles = s_handles[filepath];
    bool soundJustPlayed = false;
    for (int i = 0; i < handles.size(); ++i)
    {
        // Remove expired handles
        if (!s_core.isValidVoiceHandle(handles[i]))
        {
            handles.erase(handles.begin() + i);
            i--;
            continue;
        }

        // Catch recently played sfx
        if (s_core.getStreamPosition(handles[i]) < 0.05f)
        {
            soundJustPlayed = true;
            break;
        }
    }

    // Early out if within cooldown window
    if (soundJustPlayed) return;

    // Setup loop state before playing
    sound.setLooping(loop);
    sound.setLoopPoint(loopPoint);
    
    // Play the sound with the given arguments
    auto handle = s_core.play(sound);
    float adjustedVolume = volume;

    // Always push back handle
    handles.push_back(handle);

    // Calculate falloff if requested
    if (falloff)
    {   
        // Count active voices playing this sound
        int activeVoices = 0;
        for (int i = 0; i < handles.size(); ++i)
        {
            // Remove expired handles
            if (!s_core.isValidVoiceHandle(handles[i]))
            {
                handles.erase(handles.begin() + i);
                i--;
                continue;
            }

            // Increase valid handle count
            activeVoices++;
        }

        // Adjust volume with inverse falloff based on number of active voices
        adjustedVolume = volume / activeVoices;
    }
    
    s_core.setVolume(handle, adjustedVolume);
    s_core.setPan(handle, pan);

    // Protect background music from being killed, but allow regular sfx to be killed in case of overload
    // NOTE: Shouldn't be hardcoded here, works for this project though.
    if (filepath.starts_with("data/sounds/bgm_")) s_core.setProtectVoice(handle, true);

    // Pitch offset is simply added to the sample rate, measured in Hz
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

float Audio::GetGlobalVolume()
{
    return s_core.getGlobalVolume();
}

void Audio::SetGlobalVolume(float volume)
{
    float adjusted = glm::clamp(volume, 0.0f, 1.2f);
    s_core.setGlobalVolume(adjusted);
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