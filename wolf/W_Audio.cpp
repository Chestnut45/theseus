#include "W_Audio.h"

namespace wolf
{

void Audio::Play(const std::string& filepath, bool loop, float volume, float pan)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = s_samples[filepath];

    // Load from file on first play
    if (sound.getLength() == 0) sound.load(filepath.c_str());

    // Setup loop state before playing
    sound.setLooping(loop);

    // Play the sound with the given arguments
    auto handle = s_core.play(sound);
    s_core.setVolume(handle, volume);
    s_core.setPan(handle, pan);

}

void Audio::Stop(const std::string& filepath)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = s_samples[filepath];

    // Stop all instances of the sample
    s_core.stopAudioSource(sound);
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