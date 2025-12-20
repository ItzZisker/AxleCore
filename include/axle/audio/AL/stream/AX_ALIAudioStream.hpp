#pragma once

#include "axle/data/AX_IDataStream.hpp"
#include "axle/tick/AX_ITickAdapter.hpp"

#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace axle::audio
{

struct AudioStreamDesc {
    float bufferSeconds = 2.0f;   // ring buffer total size
    float periodSeconds = 1.0f;   // how often we refill

    bool loop = false;
};

static inline int16_t FloatToPCM16(float v) {
    v = std::clamp(v, -1.0f, 1.0f);

    // TPDF dither in range [-1, 1]
    float dither =
        (float(rand()) / RAND_MAX) -
        (float(rand()) / RAND_MAX);

    // scale dither to 1 LSB
    v += dither / 65536.0f;

    return (int16_t)std::lrintf(v * 32767.0f);
}


class ALIAudioStream : public tick::ITickAdapter {
protected:
    uint32_t m_Channels {2};
    uint32_t m_SampleRate {44100};

    AudioStreamDesc m_Desc {};

    std::vector<int16_t> m_RingBuffer {};
    size_t m_WriteCursor {0};
    size_t m_ReadCursor {0};
    size_t m_FilledSamples {0};

    bool m_EndOfStream {false};

    explicit ALIAudioStream(uint32_t channels, uint32_t sampleRate, const AudioStreamDesc& desc)
            : m_Desc(desc), m_Channels(channels), m_SampleRate(sampleRate) {
        m_RingBuffer.resize(static_cast<size_t>(sampleRate * desc.bufferSeconds * channels));
    }
public:
    virtual ~ALIAudioStream() = default;

    // Decode & fill ring buffer
    virtual void Tick(float dT) override = 0;

    // Consume decoded samples (used by audio backend)
    virtual size_t PopSamples(int16_t* out, size_t samples) = 0;

    // Seeking
    virtual void SeekSeconds(float seconds) = 0;
    virtual void SeekSamples(uint64_t sampleIndex) = 0;

    // Reset state
    virtual void Reset() = 0;

    // Info
    uint32_t GetSampleRate() const { return m_SampleRate; }
    uint32_t GetChannels() const { return m_Channels; }
    bool Ended() const { return m_EndOfStream; }
};

}