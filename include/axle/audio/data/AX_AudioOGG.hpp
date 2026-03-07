#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Span.hpp"

#include <filesystem>
#include <vector>

namespace axle::audio
{

struct OGGAudio {
    int channels;
    int sampleRate;
    std::vector<uint8_t> entireStream; 
};

utils::ExResult<OGGAudio> OGG_LoadFile(const std::filesystem::path& path);
utils::ExResult<OGGAudio> OGG_LoadFileBytes(data::IDataStream& buffer);
utils::ExResult<OGGAudio> OGG_LoadFileBytes(utils::Span<uint8_t> bufferView);

}