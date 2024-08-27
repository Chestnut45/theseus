#pragma once

//-----------------------------------------------------------------------------
// File:			W_Audio.h
// Original Author:	D'Anyil Landry
//
// A class providing simple access to audio playback for applications
//-----------------------------------------------------------------------------

#include <string>
#include <unordered_map>

#include <soloud.h>
#include <soloud_wav.h>

namespace wolf
{

class Audio
{

// Public interface
public:

    // Plays the audio from the file path specified.
    // Accepts .mp3, .WAV, .ogg, or .FLAC files.
    // 
    // File will be loaded from disk on first play, then
    // cached for all subsequent calls to Play
    static void Play(const std::string& path);

    // Loads the audio from the file path specified.
    // Accepts .mp3, .WAV, .ogg, or .FLAC files.
    // 
    // NOTE: It's not required to pre-load a file before playing,
    // but you may want to in the case of large (>100kb) files to
    // prevent a lag spike when first played.
    static void Load(const std::string& path);

    // TODO: Looping, filter options?

// Implementation
private:
    
    // SoLoud engine core
    static inline SoLoud::Soloud m_core;

    // Loaded audio sources
    static inline std::unordered_map<std::string, SoLoud::Wav> m_sources;

    // Init/Deinit functions, automatically called by wolf::App during initialization
    static void _Setup();
    static void _Shutdown();

    // Necessary for wolf::App to handle audio system setup
    friend class App;
};

}