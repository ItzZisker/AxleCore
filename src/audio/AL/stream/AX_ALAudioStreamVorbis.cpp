#include "axle/audio/AL/stream/AX_ALAudioStreamVorbis.hpp"

#include "axle/audio/AL/stream/AX_ALIAudioStream.hpp"
#include "axle/audio/data/AX_AudioOGG.hpp"

#include <iostream>
#include <ostream>

namespace axle::audio
{

ALAudioStreamVorbis::ALAudioStreamVorbis(OGGAudio& ogg, AudioStreamDesc desc)
    : m_Data(ogg), ALIAudioStream(ogg.channels, ogg.sampleRate, desc) {

    int error = 0;
    m_Vorbis = stb_vorbis_open_memory(m_Data.entireStream.data(), (int)m_Data.entireStream.size(), &error, nullptr);

    if (!m_Vorbis) {
        throw std::runtime_error("Failed to open Vorbis from memory, error=" + std::to_string(error));
    }
    m_Info = stb_vorbis_get_info(m_Vorbis);
}

ALAudioStreamVorbis::~ALAudioStreamVorbis() {
    if (m_Vorbis) {
        stb_vorbis_close(m_Vorbis);
        m_Vorbis = nullptr;
    }
}

void ALAudioStreamVorbis::Tick(float dT) {
    if (m_EndOfStream) return;

    const size_t samplesPerPeriod = size_t(m_SampleRate * m_Desc.periodSeconds) * m_Channels;

    bool print = false;
    if (m_FilledSamples < samplesPerPeriod) {
        std::cout << "prev m_FilledSamples=" << m_FilledSamples << std::endl;
        print = true;
    }
    while (m_FilledSamples < samplesPerPeriod) {
        float temp[4096];

        int frames = stb_vorbis_get_samples_float_interleaved(
            m_Vorbis,
            m_Info.channels,
            temp,
            4096
        );

        if (frames == 0) {
            if (m_Desc.loop) {
                stb_vorbis_seek_start(m_Vorbis);
                continue;
            } else {
                m_EndOfStream = true;
                break;
            }
        }

        size_t samples = frames * m_Info.channels;

        for (size_t i = 0; i < samples; i++) {
            m_RingBuffer[m_WriteCursor] = FloatToPCM16(temp[i]);
            m_WriteCursor = (m_WriteCursor + 1) % m_RingBuffer.size();
        }

        m_FilledSamples += samples;
    }
    if (print) {
        std::cout << "next m_FilledSamples=" << m_FilledSamples << std::endl;
    }
}

size_t ALAudioStreamVorbis::PopSamples(int16_t* out, size_t samples) {
    size_t count = std::min(samples, m_FilledSamples);

    for (size_t i = 0; i < count; i++) {
        out[i] = m_RingBuffer[m_ReadCursor];
        m_ReadCursor = (m_ReadCursor + 1) % m_RingBuffer.size();
    }

    m_FilledSamples -= count;
    return count;
}

void ALAudioStreamVorbis::Reset() {
    stb_vorbis_seek_start(m_Vorbis);
    m_WriteCursor = m_ReadCursor = m_FilledSamples = 0;
    m_EndOfStream = false;
}

void ALAudioStreamVorbis::SeekSeconds(float seconds) {
    uint64_t sample = uint64_t(seconds * m_SampleRate);
    SeekSamples(sample);
}

void ALAudioStreamVorbis::SeekSamples(uint64_t sampleIndex) {
    stb_vorbis_seek(m_Vorbis, sampleIndex);
    m_WriteCursor = m_ReadCursor = m_FilledSamples = 0;
    m_EndOfStream = false;
}


}