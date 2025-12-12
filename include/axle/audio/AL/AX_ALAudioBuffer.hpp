#pragma once

#include <string>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

class AX_ALAudioSource;

class AX_ALAudioBuffer
{
public:
    AX_ALAudioBuffer();
    explicit AX_ALAudioBuffer(const std::string& filePath);
    ~AX_ALAudioBuffer();

    AX_ALAudioBuffer(const AX_ALAudioBuffer&) = delete;
    AX_ALAudioBuffer& operator=(const AX_ALAudioBuffer&) = delete;

    AX_ALAudioBuffer(AX_ALAudioBuffer&& other) noexcept;
    AX_ALAudioBuffer& operator=(AX_ALAudioBuffer&& other) noexcept;

    bool LoadFromFile(const std::string& filePath); // WAV, OGG, or other supported formats
    bool LoadFromMemory(const void* data, size_t size, ALenum format, ALsizei freq);
    void Unload();

    // --- OpenAL Info ---
    ALuint GetBufferID() const { return m_bufferID; }
    ALenum GetFormat() const { return m_format; }
    ALsizei GetFrequency() const { return m_frequency; }
    ALsizei GetSize() const { return m_size; }

    void BindToSource(AX_ALAudioSource& source) const;
    void DetachFromSource(AX_ALAudioSource& source) const;

    void SetLooping(bool loop);
    bool IsLooping() const;

    void SetPitch(float pitch);
    float GetPitch() const;

    void SetGain(float gain);
    float GetGain() const;

    // optional for large audio
    bool StreamFromFile(const std::string& filePath, size_t bufferSize = 4096);
    void UpdateStream(const void* data, size_t size);

    // --- Utility / Debug ---
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

    std::string m_filePath; // Optional, for debugging and streaming
    std::vector<ALuint> m_streamBuffers; // For streaming large audio

    // Move these to AudioData
    bool LoadWAV(const std::string& filePath);
    bool LoadOGG(const std::string& filePath);
    void ClearBuffer();
};
