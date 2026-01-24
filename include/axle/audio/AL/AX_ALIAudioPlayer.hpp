#pragma once

#include "AX_ALAudioSource.hpp"
#include "buffer/AX_ALAudioBuffer.hpp"

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>
#include <mutex>

namespace axle::audio {

template <typename T>
concept T_ext_ALAudioSource = std::is_base_of_v<ALAudioSource, T>;

template <T_ext_ALAudioSource T>
class ALIAudioPlayer {
public:
    explicit ALIAudioPlayer(std::size_t maxSources = 32);
    virtual ~ALIAudioPlayer() = default;

    ALIAudioPlayer(const ALIAudioPlayer&) = delete;
    ALIAudioPlayer& operator=(const ALIAudioPlayer&) = delete;
    ALIAudioPlayer(ALIAudioPlayer&&) = delete;
    ALIAudioPlayer& operator=(ALIAudioPlayer&&) = delete;

    void SetLooping(bool loop);
    void SetPitch(float pitch);
    void SetGain(float gain);
    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);

    std::optional<T*> AcquireFreeSource();
protected:
    std::vector<std::unique_ptr<T>> m_sources;
    bool m_looping = false;
    float m_pitch = 1.0f;
    float m_gain = 1.0f;
    float m_pos[3] = {0.0f, 0.0f, 0.0f};
    float m_vel[3] = {0.0f, 0.0f, 0.0f};

private:
    std::mutex m_mutex;
};

template <T_ext_ALAudioSource T>
ALIAudioPlayer<T>::ALIAudioPlayer(std::size_t maxSources) {
    m_sources.reserve(maxSources);
    for (std::size_t i = 0; i < maxSources; ++i) {
        m_sources.emplace_back(std::make_unique<T>());
    }
}

template <T_ext_ALAudioSource T>
std::optional<T*> ALIAudioPlayer<T>::AcquireFreeSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& srcUPtr : m_sources) {
        auto* src = srcUPtr.get();
        ALint state;
        alGetSourcei(src->GetSourceID(), AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING && state != AL_PAUSED) {
            src->SetLooping(m_looping);
            src->SetPitch(m_pitch);
            src->SetGain(m_gain);
            src->SetPosition(m_pos[0], m_pos[1], m_pos[2]);
            src->SetVelocity(m_vel[0], m_vel[1], m_vel[2]);
            return src;
        }
    }
    return std::nullopt;
}

template <T_ext_ALAudioSource T>
void ALIAudioPlayer<T>::SetLooping(bool loop) {
    m_looping = loop;
    for (auto& w : m_sources) w.SetLooping(loop);
}

template <T_ext_ALAudioSource T>
void ALIAudioPlayer<T>::SetPitch(float pitch) {
    m_pitch = pitch;
    for (auto& w : m_sources) w.SetPitch(pitch);
}

template <T_ext_ALAudioSource T>
void ALIAudioPlayer<T>::SetGain(float gain) {
    m_gain = gain;
    for (auto& w : m_sources) w.SetGain(gain);
}

template <T_ext_ALAudioSource T>
void ALIAudioPlayer<T>::SetPosition(float x, float y, float z) {
    m_pos[0] = x; m_pos[1] = y; m_pos[2] = z;
    for (auto& w : m_sources) w.SetPosition(x, y, z);
}

template <T_ext_ALAudioSource T>
void ALIAudioPlayer<T>::SetVelocity(float x, float y, float z) {
    m_vel[0] = x; m_vel[1] = y; m_vel[2] = z;
    for (auto& w : m_sources) w.SetVelocity(x, y, z);
}

}
