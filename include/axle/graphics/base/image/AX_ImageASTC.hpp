#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Span.hpp"

#include <filesystem>
#include <vector>
#include <cstdint>

namespace axle::gfx {

struct ASTCHeader {
    uint8_t magic[4];    // 0x13, 0xAB, 0xA1, 0x5C -> More: https://github.com/ARM-software/astc-encoder/blob/main/Docs/FileFormat.md
    uint8_t blockdim_x;  // usually 4–12
    uint8_t blockdim_y;  // usually 4–12
    uint8_t blockdim_z;  // usually 1
    uint8_t xsize[3];    // little-endian 24-bit
    uint8_t ysize[3];
    uint8_t zsize[3];
};

struct ASTCImage {
    int width;
    int height;
    int depth;
    int blockX, blockY, blockZ;
    std::vector<uint8_t> data;
};

utils::ExResult<ASTCImage> ASTC_LoadFile(const std::filesystem::path& path);
utils::ExResult<ASTCImage> ASTC_LoadFileBytes(data::IDataStream& buffer);
utils::ExResult<ASTCImage> ASTC_LoadFileBytes(utils::Span<uint8_t> bufferView);

}