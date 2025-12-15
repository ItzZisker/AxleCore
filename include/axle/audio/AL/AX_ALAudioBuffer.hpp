#pragma once

#include "axle/audio/data/AX_AudioWAV.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <string>

namespace axle::audio {

class ALAudioSource;

// For these types of objects that hold onto driver/GPU/etc. external unique IDs,
// we should enforce unique ownership!

class ALAudioBuffer
{
public:
    ALAudioBuffer();
    ~ALAudioBuffer();

    // We are holding onto m_BufferID, copies shouldn't happen!
    ALAudioBuffer(const ALAudioBuffer&) = delete;
    ALAudioBuffer& operator=(const ALAudioBuffer&) = delete;

    // But moving is allowed
    ALAudioBuffer(ALAudioBuffer&& other) noexcept;
    ALAudioBuffer& operator=(ALAudioBuffer&& other) noexcept;

    int Load(const void* data, size_t size, ALenum format, ALsizei freq);
    int Load(const WAVAudio& wav);
    void Unload();

    ALuint GetBufferID() const { return m_bufferID; }
    ALenum GetFormat() const { return m_format; }
    ALsizei GetFrequency() const { return m_frequency; }
    ALsizei GetSize() const { return m_size; }

    void BindToSource(ALAudioSource& source) const;
    void DetachFromSource(ALAudioSource& source) const;

    void SetLooping(bool loop);
    bool IsLooping() const;

    void SetPitch(float pitch);
    float GetPitch() const;

    void SetGain(float gain);
    float GetGain() const;

    static bool IsFormatSupported(ALenum format);
    std::string GetDebugInfo() const;

private:
    ALuint m_bufferID;
    ALenum m_format; // Audio format (AL_FORMAT_MONO16, etc.)
    ALsizei m_frequency; // Sample rate
    ALsizei m_size; // Size in bytes

    bool m_looping;
    float m_pitch;
    float m_gain;

    void ClearBuffer();
};

}