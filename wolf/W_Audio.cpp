#include "W_Audio.h"

namespace wolf
{

void Audio::Play(const std::string& path)
{
    auto it = m_sources.find(path);
    SoLoud::Wav& sound = m_sources[path];

    // Load from file on first play
    if (it == m_sources.end())
    {
        sound.load(path.c_str());
    }

    m_core.play(sound);
}

void Audio::Load(const std::string& path)
{
    // Create / retrieve sound cache
    SoLoud::Wav& sound = m_sources[path];
    sound.load(path.c_str());

    // TODO: Return success status or playable sound instance?
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