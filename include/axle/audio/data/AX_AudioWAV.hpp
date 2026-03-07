#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Span.hpp"

#include <AL/al.h>

#include <filesystem>
#include <cstdint>
#include <vector>

namespace axle::audio {

// PCM only
struct WAVHeader {
    char riff[4];
    char wave[4];
    char fmt[4];
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char dataHeader[4];
    uint32_t dataSize;
};

struct WAVAudio {
    WAVHeader header;
    ALenum format;
    std::vector<char> samples;
};

utils::ExResult<WAVAudio> WAV_LoadFile(const std::filesystem::path& path);
utils::ExResult<WAVAudio> WAV_LoadFileBytes(data::IDataStream& buffer);
utils::ExResult<WAVAudio> WAV_LoadFileBytes(utils::Span<uint8_t> bufferView);

}