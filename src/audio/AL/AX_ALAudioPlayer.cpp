#include "axle/audio/AL/AX_ALAudioPlayer.hpp"

#include <AL/al.h>
#include <AL/alc.h>

namespace axle::audio {

AX_ALAudioPlayer::AX_ALAudioPlayer(size_t maxSources) {
    m_sources.resize(maxSources);
}

AX_ALAudioPlayer::~AX_ALAudioPlayer() {}

AX_ALAudioPlayer::SourceWrapper* AX_ALAudioPlayer::AcquireFreeSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& wrapper : m_sources) {
        ALint state;
        alGetSourcei(wrapper.source.GetSourceID(), AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING && state != AL_PAUSED) {
            wrapper.inUse = true;
            wrapper.source.SetLooping(m_looping);
            wrapper.source.SetPitch(m_pitch);
            wrapper.source.SetGain(m_gain);
            wrapper.source.SetPosition(m_pos[0], m_pos[1], m_pos[2]);
            wrapper.source.SetVelocity(m_vel[0], m_vel[1], m_vel[2]);
            return &wrapper;
        }
    }
    return nullptr; // no free source
}

void AX_ALAudioPlayer::PlaySound(AX_ALAudioBuffer& buffer) {
    auto srcWrapper = AcquireFreeSource();
    if (!srcWrapper) return; // all sources busy

    srcWrapper->source.AttachBuffer(buffer);
    srcWrapper->source.Play();
}

void AX_ALAudioPlayer::SetLooping(bool loop) {
    m_looping = loop;
    for (auto& w : m_sources) w.source.SetLooping(loop);
}

void AX_ALAudioPlayer::SetPitch(float pitch) {
    m_pitch = pitch;
    for (auto& w : m_sources) w.source.SetPitch(pitch);
}

void AX_ALAudioPlayer::SetGain(float gain) {
    m_gain = gain;
    for (auto& w : m_sources) w.source.SetGain(gain);
}

void AX_ALAudioPlayer::SetPosition(float x, float y, float z) {
    m_pos[0] = x; m_pos[1] = y; m_pos[2] = z;
    for (auto& w : m_sources) w.source.SetPosition(x, y, z);
}

void AX_ALAudioPlayer::SetVelocity(float x, float y, float z) {
    m_vel[0] = x; m_vel[1] = y; m_vel[2] = z;
    for (auto& w : m_sources) w.source.SetVelocity(x, y, z);
}

};