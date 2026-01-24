#pragma once

#include "axle/data/AX_DataDeserializer.hpp"

#include <filesystem>
#include <vector>

namespace axle::audio
{

struct OGGAudio {
    int channels;
    int sampleRate;
    std::vector<uint8_t> entireStream; 
};

OGGAudio OGG_LoadFile(const std::filesystem::path& path);
OGGAudio OGG_LoadFileBytes(data::DataDeserializer* buffer);
OGGAudio OGG_LoadFileBytes(const uint8_t* bytes, int length);

bool OGG_isValidFileBytes(data::DataDeserializer* buffer);
bool OGG_isValidFileBytes(const uint8_t* bytes, int length);
bool OGG_isValidFile(const std::filesystem::path& path);

}