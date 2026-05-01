#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Span.hpp"

#include <filesystem>
#include <cstdint>

namespace axle::gfx
{

enum class ImageFormat {
    Raw_R8,
    Raw_RG8,
    Raw_RGB8,
    Raw_RGBA8,

    Raw_R16,
    Raw_RG16,
    Raw_RGB16,
    Raw_RGBA16,

    Raw_R32F,
    Raw_RG32F,
    Raw_RGB32F,
    Raw_RGBA32F,

    Compressed_RGBA_ASTC_4x4,
    Compressed_RGBA_ASTC_5x5,
    Compressed_RGBA_ASTC_6x6,
    Compressed_RGBA_ASTC_8x8,

    Compressed_RGB_DXT1,
    Compressed_RGBA_DXT5
};

bool Img_IsCompressed(const ImageFormat& fmt);

uint32_t Img_GetChannelCount(const ImageFormat& fmt);
uint32_t Img_GetBytesPerChannel(const ImageFormat& fmt);

struct Image {
    ImageFormat format;
    int width, height;
    utils::URaw bytes;

    inline size_t GetSize() const {
        if (Img_IsCompressed(format)) {
            return bytes.size();
        } else {
            return static_cast<std::size_t>(width) * height
                * Img_GetChannelCount(format)
                * Img_GetBytesPerChannel(format);
        }
    }
};

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
    utils::URaw data;
};

struct BCnImage {
    uint16_t width;
    uint16_t height;
    uint8_t alpha; // 0 = BC1/DXT1, 1 = BC3/DXT5
    utils::URaw data;
};

bool Img_ASTC_IsValidFile(const std::filesystem::path& path);
bool Img_ASTC_IsValidFileBytes(data::IDataStream& buffer);
bool Img_ASTC_IsValidFileBytes(utils::URawView bufferView);

utils::ExResult<ASTCImage> Img_ASTC_LoadFile(const std::filesystem::path& path);
utils::ExResult<ASTCImage> Img_ASTC_LoadFileBytes(data::IDataStream& buffer);
utils::ExResult<ASTCImage> Img_ASTC_LoadFileBytes(utils::URawView bufferView);

bool Img_BCn_IsValidFile(const std::filesystem::path& path);
bool Img_BCn_IsValidFileBytes(data::IDataStream& buffer);
bool Img_BCn_IsValidFileBytes(utils::URawView bufferView);

utils::ExResult<BCnImage> Img_BCn_LoadFile(const std::filesystem::path& path);
utils::ExResult<BCnImage> Img_BCn_LoadFileBytes(data::IDataStream& buffer);
utils::ExResult<BCnImage> Img_BCn_LoadFileBytes(utils::URawView bufferView);

utils::ExResult<Image> Img_Auto_LoadFile(const std::filesystem::path& path);
utils::ExResult<Image> Img_Auto_LoadFileBytes(data::IDataStream& buffer);
utils::ExResult<Image> Img_Auto_LoadFileBytes(utils::URawView bufferView);

}