#pragma once

#include "AX_ALAudioBuffer.hpp"
#include "AX_ALAudioSource.hpp"

#include <vector>
#include <mutex>

namespace axle::audio {

class AX_ALAudioPlayer
{
public:
    AX_ALAudioPlayer(size_t maxSources = 32);
    ~AX_ALAudioPlayer();

    AX_ALAudioPlayer(const AX_ALAudioPlayer&) = delete;
    AX_ALAudioPlayer& operator=(const AX_ALAudioPlayer&) = delete;

    AX_ALAudioPlayer(AX_ALAudioPlayer&&) = delete;
    AX_ALAudioPlayer& operator=(AX_ALAudioPlayer&&) = delete;

    void PlaySound(AX_ALAudioBuffer& buffer);

    void SetLooping(bool loop);
    void SetPitch(float pitch);
    void SetGain(float gain);
    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);
private:
    struct SourceWrapper {
        AX_ALAudioSource source;
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