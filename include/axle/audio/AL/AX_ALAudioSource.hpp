#pragma once

#include "buffer/AX_ALAudioBuffer.hpp"

#include <AL/al.h>
#include <AL/alc.h>

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

    // Configure for music, bypass HRTF (3D/Doppler/etc. Audio Effects) filter
    void BypassHRTF();

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

    ALuint GetSourceID() const { return m_sourceID; }
protected:
    ALuint m_sourceID{0};

    bool m_looping{false};
    float m_pitch{1.0f};
    float m_gain{1.0f};
};

}
