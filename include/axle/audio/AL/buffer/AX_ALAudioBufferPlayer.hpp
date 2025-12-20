#pragma once

#include "axle/audio/AL/AX_ALIAudioPlayer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBufferSource.hpp"

#include <functional>

namespace axle::audio
{

class ALAudioBufferPlayer : public ALIAudioPlayer<ALAudioBufferSource> {
public:
    ALAudioBufferPlayer(size_t max_sources = 32);

    ALAudioBufferSource& Play(ALAudioBuffer& buffer);
    void ApplyToSources(std::function<void(ALAudioBufferSource&)> iteration_func);
};

}