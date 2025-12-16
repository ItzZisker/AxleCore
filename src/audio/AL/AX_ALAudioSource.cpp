#include "axle/audio/AL/AX_ALAudioSource.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <ostream>

namespace axle::audio {

ALAudioSource::ALAudioSource() : m_looping(false), m_pitch(1.0f), m_gain(1.0f) {
    alGenSources(1, &m_sourceID);
    alSourcei(m_sourceID, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcef(m_sourceID, AL_PITCH, m_pitch);
    alSourcef(m_sourceID, AL_GAIN, m_gain);
    alSourcei(m_sourceID, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);
}

ALAudioSource::~ALAudioSource() {
    if (m_sourceID) alDeleteSources(1, &m_sourceID);
}

ALAudioSource::ALAudioSource(ALAudioSource&& other) noexcept {
    *this = std::move(other);
}

ALAudioSource& ALAudioSource::operator=(ALAudioSource&& other) noexcept {
    if (this == &other) return *this;
    if (m_sourceID) alDeleteSources(1, &m_sourceID);
    m_sourceID = other.m_sourceID;
    m_looping = other.m_looping;
    m_pitch = other.m_pitch;
    m_gain = other.m_gain;
    other.m_sourceID = 0;
    return *this;
}

void ALAudioSource::Play() {
    if (m_sourceID) alSourcePlay(m_sourceID);
}

void ALAudioSource::Pause() {
    if (m_sourceID) alSourcePause(m_sourceID);
}

void ALAudioSource::Stop() {
    if (m_sourceID) alSourceStop(m_sourceID);
}

void ALAudioSource::Purge() {
    if (m_sourceID) alDeleteSources(1, &m_sourceID);
}

bool ALAudioSource::IsPlaying() const {
    if (!m_sourceID) return false;
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

bool ALAudioSource::IsPaused() const {
    if (!m_sourceID) return false;
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    return state == AL_PAUSED;
}

bool ALAudioSource::IsStopped() const {
    if (!m_sourceID) return true;
    ALint state;
    alGetSourcei(m_sourceID, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}

void ALAudioSource::SetLooping(bool loop) {
    m_looping = loop;
    if (!m_sourceID) return;
    alSourcei(m_sourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

void ALAudioSource::SetPitch(float pitch) {
    m_pitch = pitch;
    if (!m_sourceID) return;
    alSourcef(m_sourceID, AL_PITCH, pitch);
}

void ALAudioSource::SetGain(float gain) {
    m_gain = gain;
    if (!m_sourceID) return;
    alSourcef(m_sourceID, AL_GAIN, gain);
}

void ALAudioSource::SetPosition(float x, float y, float z) {
    if (!m_sourceID) return;
    alSource3f(m_sourceID, AL_POSITION, x, y, z);
}

void ALAudioSource::SetVelocity(float x, float y, float z) {
    if (!m_sourceID) return;
    alSource3f(m_sourceID, AL_VELOCITY, x, y, z);
}

void ALAudioSource::AttachBuffer(const ALAudioBuffer& buffer) {
    if (!m_sourceID) return;
    alSourcei(m_sourceID, AL_BUFFER, (ALint)buffer.GetBufferID());
}

// Only detach if the same buffer is bound
void ALAudioSource::DetachBuffer(const ALAudioBuffer& buffer) {
    if (!m_sourceID) return;
    ALint bound;
    alGetSourcei(m_sourceID, AL_BUFFER, &bound);
    if ((ALuint)bound == buffer.GetBufferID()) {
        alSourcei(m_sourceID, AL_BUFFER, 0);
    }
}

bool ALAudioSource::IsLooping() const {
    return m_looping;
}

float ALAudioSource::GetPitch() const {
    return m_pitch;
}

float ALAudioSource::GetGain() const {
    return m_gain;
}

}