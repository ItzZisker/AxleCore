#pragma once

#include "axle/audio/AL/AX_ALIAudioPlayer.hpp"

#include "axle/audio/AL/stream/AX_ALAudioStreamVorbis.hpp"
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisSource.hpp"

#include "axle/tick/AX_ITickAdapter.hpp"

#include <cstddef>
#include <functional>

namespace axle::audio
{

class ALAudioStreamVorbisPlayer : public ALIAudioPlayer<ALAudioStreamVorbisSource>, public tick::ITickAdapter {
public:
    ALAudioStreamVorbisPlayer(std::size_t max_sources = 4);

    ALAudioStreamVorbisSource* Play(ALAudioStreamVorbis* stream);
    void ApplyToSources(std::function<void(ALAudioStreamVorbisSource&)> iteration_func);

    void Tick(float dT) override; // Tick AudioStream(s)
};

}
