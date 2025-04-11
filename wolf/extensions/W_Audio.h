#pragma once

//-----------------------------------------------------------------------------
// File:			W_Audio.h
// Original Author:	D'Anyil Landry
//
// A static class providing simple access to audio playback for applications.
// Supports pitch shifting, stereo pan, looping, and optional volume falloff.
// 
// NOTE: Crashes on initialization for specific output devices...
// Seems like an incomplete Miniaudio implementation for certain formats.
//-----------------------------------------------------------------------------

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <soloud.h>
#include <soloud_wav.h>

namespace wolf
{

typedef uint32_t AudioID;

class Audio
{
    // Necessary for wolf::App to handle audio system setup
    friend class App;

// Public interface
public:

    // Plays an audio sample from the filepath specified.
    // Accepts .mp3, .WAV, .ogg, or .FLAC files.
    // If not already loaded, the audio sample will be
    // loaded from disk and cached for subsequent use.
    // 
    // NOTE: Multiple instances of the same sample can
    // be played simultaneously with different arguments.
    static void Play(const std::string& filepath, float volume = 1.0f, float pitchOffset = 0.0f, float pan = 0.0f, bool falloff = false, bool loop = false, float loopPoint = 0.0f);

    // Stops all instances of an audio sample that are currently playing
    static void Stop(const std::string& filepath);

    // Stops ALL audio instantly
    static void Stop();

    // Loads an audio sample from the filepath specified.
    // Accepts .mp3, .WAV, .ogg, or .FLAC files.
    // 
    // NOTE: It's not required to pre-load a file before playing,
    // but you may want to in the case of large (>100kb) files to
    // prevent a lag spike when first played.
    static void Load(const std::string& filepath);

    // TODO: Filters? (reverb, delay, bitcrushing, etc.)

// Implementation
private:
    
    // SoLoud engine core
    static inline SoLoud::Soloud s_core;

    // Loaded audio samples
    static inline std::unordered_map<std::string, SoLoud::Wav> s_samples;

    // Map of filepaths to possibly active handles used for volume normalization
    static inline std::unordered_map<std::string, std::vector<SoLoud::handle>> s_handles;

    // Init/Deinit functions, automatically called by wolf::App during initialization
    static void _Setup();
    static void _Shutdown();
};

}
