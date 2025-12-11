#pragma once

#include "axle/data/AX_DataDeserializer.hpp"

#include <filesystem>
#include <vector>
#include <cstdint>

namespace axle::graphics {

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

ASTCImage ASTC_LoadFile(const std::filesystem::path& path);
ASTCImage ASTC_LoadFileBytes(data::DataDeserializer *buffer);
ASTCImage ASTC_LoadFileBytes(const uint8_t* bytes, int length);
ASTCImage ASTC_LoadASTC_File(const std::filesystem::path& path);

bool ASTC_isValidFileBytes(data::DataDeserializer *buffer);
bool ASTC_isValidFileBytes(const uint8_t* bytes, int length);
bool ASTC_isValidFile(const std::filesystem::path& path);

}