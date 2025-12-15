#pragma once

#include "axle/data/AX_DataDeserializer.hpp"

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

WAVAudio WAV_LoadFile(const std::filesystem::path& path);
WAVAudio WAV_LoadFileBytes(data::DataDeserializer *buffer);
WAVAudio WAV_LoadFileBytes(const uint8_t* bytes, int length);

bool WAV_isValidFileBytes(data::DataDeserializer *buffer);
bool WAV_isValidFileBytes(const uint8_t* bytes, int length);
bool WAV_isValidFile(const std::filesystem::path& path);

}