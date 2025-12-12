#include "axle/audio/AL/AX_ALAudioSource.hpp"

#include <AL/al.h>
#include <AL/alc.h>

namespace axle::audio {

AX_ALAudioSource::AX_ALAudioSource() : m_looping(false), m_pitch(1.0f), m_gain(1.0f) {
    alGenSources(1, &m_sourceID);
    alSourcef(m_sourceID, AL_PITCH, m_pitch);
    alSourcef(m_sourceID, AL_GAIN, m_gain);
    alSourcei(m_sourceID, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);
}

AX_ALAudioSource::~AX_ALAudioSource() {
    if (m_sourceID) alDeleteSources(1, &m_sourceID);
}

AX_ALAudioSource::AX_ALAudioSource(AX_ALAudioSource&& other) noexcept {
    *this = std::move(other);
}

AX_ALAudioSource& AX_ALAudioSource::operator=(AX_ALAudioSource&& other) noexcept {
    if (this == &other) return *this;
    if (m_sourceID) alDeleteSources(1, &m_sourceID);
    m_sourceID = other.m_sourceID;
    m_looping = other.m_looping;
    m_pitch = other.m_pitch;
    m_gain = other.m_gain;
    other.m_sourceID = 0;
}

void AX_ALAudioSource::Play() {
    alSourcePlay(m_sourceID);
}

void AX_ALAudioSource::Pause() {
    alSourcePause(m_sourceID);
}

void AX_ALAudioSource::Stop() {
    alSourceStop(m_sourceID);
}

bool AX_ALAudioSource::IsPlaying() const {
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

bool AX_ALAudioSource::IsPaused() const {
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    return state == AL_PAUSED;
}

bool AX_ALAudioSource::IsStopped() const {
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}

void AX_ALAudioSource::SetLooping(bool loop) {
    m_looping = loop;
    alSourcei(m_sourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

void AX_ALAudioSource::SetPitch(float pitch) {
    m_pitch = pitch;
    alSourcef(m_sourceID, AL_PITCH, pitch);
}

void AX_ALAudioSource::SetGain(float gain) {
    m_gain = gain;
    alSourcef(m_sourceID, AL_GAIN, gain);
}

void AX_ALAudioSource::SetPosition(float x, float y, float z) {
    alSource3f(m_sourceID, AL_POSITION, x, y, z);
}

void AX_ALAudioSource::SetVelocity(float x, float y, float z) {
    alSource3f(m_sourceID, AL_VELOCITY, x, y, z);
}

void AX_ALAudioSource::AttachBuffer(const AX_ALAudioBuffer& buffer) {
    alSourcei(m_sourceID, AL_BUFFER, (ALint)buffer.GetBufferID());
}

// Only detach if the same buffer is bound
void AX_ALAudioSource::DetachBuffer(const AX_ALAudioBuffer& buffer) {
    ALint bound;
    alGetSourcei(m_sourceID, AL_BUFFER, &bound);
    if ((ALuint)bound == buffer.GetBufferID()) {
        alSourcei(m_sourceID, AL_BUFFER, 0);
    }
}

bool AX_ALAudioSource::IsLooping() const {
    return m_looping;
}

float AX_ALAudioSource::GetPitch() const {
    return m_pitch;
}

float AX_ALAudioSource::GetGain() const {
    return m_gain;
}

}