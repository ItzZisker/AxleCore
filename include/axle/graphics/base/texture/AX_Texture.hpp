#pragma once

#include <cstddef>
#include <vector>
#include <cstdint>

namespace axle::graphics {

enum ImageFormat {
    R8 = 0,       // 1 channel, 8-bit
    RG8 = 1,      // 2 channels, 8-bit
    RGB8 = 2,     // 3 channels, 8-bit
    RGBA8 = 3,    // 4 channels, 8-bit

    R16 = 0,       // 1 channel, 16-bit
    RG16 = 1,      // 2 channels, 16-bit
    RGB16 = 2,     // 3 channels, 16-bit
    RGBA16 = 3,    // 4 channels, 16-bit

    // Below is definition of EXR/HDR formats. High dynamic-range colors often used for textures such as BRDF_LUT in which color precision matters for later calculations
    R32F = 8,     // 1 channel, 32-bit float
    RG32F = 9,    // 2 channels, 32-bit float
    RGB32F = 10,  // 3 channels, 32-bit float
    RGBA32F = 11, // 4 channels, 32-bit float
};

enum ImageFormatInternal {
    Base_R,
    Base_RG,
    Base_RGB,
    Base_RGBA,

    // Below is definition of ASTC block-compressed formats that could be uploaded directly to any GLES GPUs while maintaining fast load time.
    Compressed_RGBA_ASTC_4x4,
    Compressed_RGBA_ASTC_5x5,
    Compressed_RGBA_ASTC_6x6,
    Compressed_RGBA_ASTC_8x8,

    // Below is definition of BC compressed formats that could be uploaded directly to any GL-Desktop GPUs while maintaining fast load time. (DXT1/5)
    Compressed_RGB_DXT1,
    Compressed_RGBA_DXT5
};

enum ImagePixelType {
    Float32,
    Float16,
    UByte
};

struct ImageFormatInfo {
    ImageFormat format;
    ImageFormatInternal internalFormat;
    ImagePixelType type;
    int nrComponents;

    size_t bits() const {
        return type == Float16 ? 2 : type == Float32 ? 4 : 1;
    }
};

ImageFormatInfo Tx_GetFormatInfo(ImageFormat fmt);
ImageFormat Tx_GetImageFormat(int nrChannels, int bits, bool isHDR);

struct TextureImage {
    ImageFormat format = ImageFormat::RGB8;
    std::vector<uint8_t> bytes;
    int width, height;

    ImageFormatInfo GetInfo() const {
        return Tx_GetFormatInfo(format);
    }
    size_t GetSize() const {
        const ImageFormatInfo& info = GetInfo();
        if (static_cast<int>(info.internalFormat) >= ImageFormatInternal::Compressed_RGBA_ASTC_4x4) return bytes.size();
        return static_cast<size_t>(width * height * info.nrComponents) * info.bits();
    }
};

}