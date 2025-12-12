#pragma once

#include "AX_ALAudioBuffer.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <string>
#include <vector>

namespace axle::audio {

class AX_ALAudioSource
{
public:
    AX_ALAudioSource();
    ~AX_ALAudioSource();

    AX_ALAudioSource(const AX_ALAudioSource&) = delete;
    AX_ALAudioSource& operator=(const AX_ALAudioSource&) = delete;

    AX_ALAudioSource(AX_ALAudioSource&& other) noexcept;
    AX_ALAudioSource& operator=(AX_ALAudioSource&& other) noexcept;

    void Play();
    void Pause();
    void Stop();

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

    void AttachBuffer(const AX_ALAudioBuffer& buffer);
    void DetachBuffer(const AX_ALAudioBuffer& buffer);

    ALuint GetSourceID() const { return m_sourceID; }
private:
    ALuint m_sourceID;

    bool m_looping;
    float m_pitch;
    float m_gain;
};

}