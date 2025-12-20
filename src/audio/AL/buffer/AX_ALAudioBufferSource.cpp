#include "axle/audio/AL/buffer/AX_ALAudioBufferSource.hpp"

namespace axle::audio
{

void ALAudioBufferSource::AttachBuffer(const ALAudioBuffer& buffer) {
    if (!m_sourceID) return;
    alSourcei(m_sourceID, AL_BUFFER, (ALint)buffer.GetBufferID());
}

// Only detach if the same buffer is bound
void ALAudioBufferSource::DetachBuffer(const ALAudioBuffer& buffer) {
    if (!m_sourceID) return;
    ALint bound;
    alGetSourcei(m_sourceID, AL_BUFFER, &bound);
    if ((ALuint)bound == buffer.GetBufferID()) {
        alSourcei(m_sourceID, AL_BUFFER, 0);
    }
}

}