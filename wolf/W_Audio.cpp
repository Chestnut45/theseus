#include "W_Audio.h"

namespace wolf
{

void Audio::Play(const std::string& filepath, bool loop, float volume, float pan)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = m_samples[filepath];

    // Load from file on first play
    if (sound.getLength() == 0) sound.load(filepath.c_str());

    // Setup loop state before playing
    sound.setLooping(loop);

    // Play the sound with the given arguments
    auto handle = m_core.play(sound);
    m_core.setVolume(handle, volume);
    m_core.setPan(handle, pan);

}

void Audio::Stop(const std::string& filepath)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = m_samples[filepath];

    // Stop all instances of the sample
    m_core.stopAudioSource(sound);
}

void Audio::Load(const std::string& filepath)
{
    // Create / retrieve sample
    SoLoud::Wav& sound = m_samples[filepath];

    // Ensure no instances are playing this sound
    // Loading during playback can crash
    m_core.stopAudioSource(sound);

    sound.load(filepath.c_str());
}

void Audio::_Setup()
{
    m_core.init();
}

void Audio::_Shutdown()
{
    m_core.stopAll();
    m_core.deinit();
}

}