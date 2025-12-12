#include "axle/audio/AL/AX_ALAudioBuffer.hpp"
#include "axle/audio/AL/AX_ALAudioSource.hpp"

#include "axle/audio/data/AX_AudioWAV.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <vector>
#include <string>
#include <cstring>

namespace axle::audio {

AX_ALAudioBuffer::AX_ALAudioBuffer() :
    m_bufferID(0), m_format(0), m_frequency(0), m_size(0),
    m_looping(false), m_pitch(1.0f), m_gain(1.0f) {
    alGenBuffers(1, &m_bufferID);
}

AX_ALAudioBuffer::~AX_ALAudioBuffer() {
    Unload();
}

AX_ALAudioBuffer::AX_ALAudioBuffer(AX_ALAudioBuffer &&other) noexcept {
    m_bufferID = other.m_bufferID;
    m_format = other.m_format;
    m_frequency = other.m_frequency;
    m_size = other.m_size;
    m_looping = other.m_looping;
    m_pitch = other.m_pitch;
    m_gain = other.m_gain;

    other.m_bufferID = 0;
}

AX_ALAudioBuffer &AX_ALAudioBuffer::operator=(AX_ALAudioBuffer &&other) noexcept {
    if (this == &other) return *this;
    Unload();
    m_bufferID = other.m_bufferID;
    m_format = other.m_format;
    m_frequency = other.m_frequency;
    m_size = other.m_size;
    m_looping = other.m_looping;
    m_pitch = other.m_pitch;
    m_gain = other.m_gain;

    other.m_bufferID = 0;
    return *this;
}

bool AX_ALAudioBuffer::Load(const void *data, size_t size, ALenum format, ALsizei freq) {
    ClearBuffer();
    m_format = format;
    m_frequency = freq;
    m_size = static_cast<ALsizei>(size);
    alBufferData(m_bufferID, format, data, m_size, freq);
    return (alGetError() == AL_NO_ERROR);
}

bool AX_ALAudioBuffer::Load(const WAVAudio& wav) {
    AX_ALAudioBuffer::Load(
        wav.samples.data(),
        wav.samples.size(),
        wav.format,
        static_cast<ALsizei>(wav.header.sampleRate)
    );
}

void AX_ALAudioBuffer::Unload() {
    if (m_bufferID != 0) {
        alDeleteBuffers(1, &m_bufferID);
        m_bufferID = 0;
    }
}

void AX_ALAudioBuffer::BindToSource(AX_ALAudioSource &source) const {
    alSourcei(source.GetSourceID(), AL_BUFFER, m_bufferID);
}

void AX_ALAudioBuffer::DetachFromSource(AX_ALAudioSource &source) const {
    alSourcei(source.GetSourceID(), AL_BUFFER, 0);
}

void AX_ALAudioBuffer::SetLooping(bool loop) {
    m_looping = loop;
}

bool AX_ALAudioBuffer::IsLooping() const {
    return m_looping;
}

void AX_ALAudioBuffer::SetPitch(float pitch) {
    m_pitch = pitch;
}

float AX_ALAudioBuffer::GetPitch() const {
    return m_pitch;
}

void AX_ALAudioBuffer::SetGain(float gain) {
    m_gain = gain;
}

float AX_ALAudioBuffer::GetGain() const {
    return m_gain;
}

bool AX_ALAudioBuffer::IsFormatSupported(ALenum format) {
    return (format == AL_FORMAT_MONO8 || format == AL_FORMAT_MONO16 ||
            format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16);
}

std::string AX_ALAudioBuffer::GetDebugInfo() const {
    return "AudioBuffer { id=" + std::to_string(m_bufferID) +
           ", size=" + std::to_string(m_size) +
           ", freq=" + std::to_string(m_frequency) +
           ", format=" + std::to_string(m_format) + " }";
}

void AX_ALAudioBuffer::ClearBuffer() {
    if (m_bufferID != 0) {
        alBufferData(m_bufferID, AL_FORMAT_MONO16, nullptr, 0, 44100);
    }
}

}