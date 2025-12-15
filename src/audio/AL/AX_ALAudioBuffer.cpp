#include "axle/audio/AL/AX_ALAudioBuffer.hpp"
#include "axle/audio/AL/AX_ALAudioSource.hpp"

#include "axle/audio/data/AX_AudioWAV.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <iostream>
#include <ostream>
#include <vector>
#include <string>
#include <cstring>

namespace axle::audio {

ALAudioBuffer::ALAudioBuffer() :
    m_bufferID(0), m_format(0), m_frequency(0), m_size(0),
    m_looping(false), m_pitch(1.0f), m_gain(1.0f) {
    alGenBuffers(1, &m_bufferID);
}

ALAudioBuffer::~ALAudioBuffer() {
    Unload();
}

ALAudioBuffer::ALAudioBuffer(ALAudioBuffer &&other) noexcept {
    m_bufferID = other.m_bufferID;
    m_format = other.m_format;
    m_frequency = other.m_frequency;
    m_size = other.m_size;
    m_looping = other.m_looping;
    m_pitch = other.m_pitch;
    m_gain = other.m_gain;

    other.m_bufferID = 0;
}

ALAudioBuffer &ALAudioBuffer::operator=(ALAudioBuffer &&other) noexcept {
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

int ALAudioBuffer::Load(const void *data, size_t size, ALenum format, ALsizei freq) {
    ClearBuffer();
    m_format = format;
    m_frequency = freq;
    m_size = static_cast<ALsizei>(size);
    alBufferData(m_bufferID, format, data, m_size, freq);
    ALint channels;
    alGetBufferi(m_bufferID, AL_CHANNELS, &channels);
    std::cout << "Channels = " << channels << std::endl;
    return (alGetError()); // TODO: Cleaner error wrapper, just like GL in future.
}

int ALAudioBuffer::Load(const WAVAudio& wav) {
    return ALAudioBuffer::Load(
        wav.samples.data(),
        wav.samples.size(),
        wav.format,
        static_cast<ALsizei>(wav.header.sampleRate)
    );
}

void ALAudioBuffer::Unload() {
    if (m_bufferID != 0) {
        alDeleteBuffers(1, &m_bufferID);
        m_bufferID = 0;
    }
}

void ALAudioBuffer::BindToSource(ALAudioSource &source) const {
    std::cout << "srcID=" << source.GetSourceID() << std::endl;
    std::cout << "m_bufferID=" << m_bufferID << std::endl;
    alSourcei(source.GetSourceID(), AL_BUFFER, m_bufferID);
}

void ALAudioBuffer::DetachFromSource(ALAudioSource &source) const {
    alSourcei(source.GetSourceID(), AL_BUFFER, 0);
}

void ALAudioBuffer::SetLooping(bool loop) {
    m_looping = loop;
}

bool ALAudioBuffer::IsLooping() const {
    return m_looping;
}

void ALAudioBuffer::SetPitch(float pitch) {
    m_pitch = pitch;
}

float ALAudioBuffer::GetPitch() const {
    return m_pitch;
}

void ALAudioBuffer::SetGain(float gain) {
    m_gain = gain;
}

float ALAudioBuffer::GetGain() const {
    return m_gain;
}

bool ALAudioBuffer::IsFormatSupported(ALenum format) {
    return (format == AL_FORMAT_MONO8 || format == AL_FORMAT_MONO16 ||
            format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16);
}

std::string ALAudioBuffer::GetDebugInfo() const {
    return "AudioBuffer { id=" + std::to_string(m_bufferID) +
           ", size=" + std::to_string(m_size) +
           ", freq=" + std::to_string(m_frequency) +
           ", format=" + std::to_string(m_format) + " }";
}

void ALAudioBuffer::ClearBuffer() {
    if (m_bufferID != 0) {
        alBufferData(m_bufferID, AL_FORMAT_MONO16, nullptr, 0, 44100);
    }
}

}