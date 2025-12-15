#pragma once

#include "AX_ALAudioBuffer.hpp"
#include "AX_ALAudioSource.hpp"

#include <vector>
#include <mutex>

namespace axle::audio {

class ALAudioPlayer
{
public:
    ALAudioPlayer(std::size_t maxSources = 32);
    ~ALAudioPlayer();

    ALAudioPlayer(const ALAudioPlayer&) = delete;
    ALAudioPlayer& operator=(const ALAudioPlayer&) = delete;

    ALAudioPlayer(ALAudioPlayer&&) = delete;
    ALAudioPlayer& operator=(ALAudioPlayer&&) = delete;

    void PlaySound(ALAudioBuffer& buffer);

    void SetLooping(bool loop);
    void SetPitch(float pitch);
    void SetGain(float gain);
    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);
private:
    struct SourceWrapper {
        ALAudioSource source;
        bool inUse = false;
    };

    std::vector<SourceWrapper> m_sources;
    std::mutex m_mutex;

    // global config applied to all sources
    bool m_looping = false;
    float m_pitch = 1.0f;
    float m_gain = 1.0f;
    float m_pos[3] = {0.0f, 0.0f, 0.0f};
    float m_vel[3] = {0.0f, 0.0f, 0.0f};

    // Nullable!!!
    SourceWrapper* AcquireFreeSource();
};

}