#pragma once

#include "AX_ALAudioBuffer.hpp"
#include "../AX_ALAudioSource.hpp"

namespace axle::audio
{

class ALAudioBufferSource : public ALAudioSource {
public:
    void AttachBuffer(const ALAudioBuffer& buffer);
    void DetachBuffer(const ALAudioBuffer& buffer);
};

}
