#pragma once

#include "AX_ALAudioBuffer.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <string>
#include <vector>

namespace axle::audio {

class ALAudioSource
{
public:
    ALAudioSource();
    ~ALAudioSource();

    ALAudioSource(const ALAudioSource&) = delete;
    ALAudioSource& operator=(const ALAudioSource&) = delete;

    ALAudioSource(ALAudioSource&& other) noexcept;
    ALAudioSource& operator=(ALAudioSource&& other) noexcept;

    void Play();
    void Pause();
    void Stop();
    void Purge();

    bool IsPlaying() const;
    bool IsLooping() const;
    bool IsPaused() const;
    bool IsStopped() const;

    void SetLooping(bool loop);
    void SetPitch(float pitch);
    void SetGain(float gain);
    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);

    float GetPitch() const;
    float GetGain() const;

    void AttachBuffer(const ALAudioBuffer& buffer);
    void DetachBuffer(const ALAudioBuffer& buffer);

    ALuint GetSourceID() const { return m_sourceID; }
private:
    ALuint m_sourceID;

    bool m_looping;
    float m_pitch;
    float m_gain;
};

}