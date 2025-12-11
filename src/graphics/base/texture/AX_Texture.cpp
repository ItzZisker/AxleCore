#include "axle/graphics/base/texture/AX_Texture.hpp"

#include <array>
#include <stdexcept>

namespace axle::graphics {

ImageFormatInfo Tx_GetFormatInfo(ImageFormat fmt) {
    static std::array<ImageFormatInfo, 18> ImageFormatInfoMap = {{
        {ImageFormat::R8,    ImageFormatInternal::Base_R,    ImagePixelType::UByte, 1},
        {ImageFormat::RG8,   ImageFormatInternal::Base_RG,   ImagePixelType::UByte, 2},
        {ImageFormat::RGB8,  ImageFormatInternal::Base_RGB,  ImagePixelType::UByte, 3},
        {ImageFormat::RGBA8, ImageFormatInternal::Base_RGBA, ImagePixelType::UByte, 4},

        {ImageFormat::R16,    ImageFormatInternal::Base_R,    ImagePixelType::UByte, 1},
        {ImageFormat::RG16,   ImageFormatInternal::Base_RG,   ImagePixelType::UByte, 2},
        {ImageFormat::RGB16,  ImageFormatInternal::Base_RGB,  ImagePixelType::UByte, 3},
        {ImageFormat::RGBA16, ImageFormatInternal::Base_RGBA, ImagePixelType::UByte, 4},

        {ImageFormat::R32F,    ImageFormatInternal::Base_R,    ImagePixelType::Float32, 1},
        {ImageFormat::RG32F,   ImageFormatInternal::Base_RG,   ImagePixelType::Float32, 2},
        {ImageFormat::RGB32F,  ImageFormatInternal::Base_RGB,  ImagePixelType::Float32, 3},
        {ImageFormat::RGBA32F, ImageFormatInternal::Base_RGBA, ImagePixelType::Float32, 4},

        {ImageFormat::RGBA8, ImageFormatInternal::Compressed_RGBA_ASTC_4x4, ImagePixelType::UByte, 4},
        {ImageFormat::RGBA8, ImageFormatInternal::Compressed_RGBA_ASTC_5x5, ImagePixelType::UByte, 4},
        {ImageFormat::RGBA8, ImageFormatInternal::Compressed_RGBA_ASTC_6x6, ImagePixelType::UByte, 4},
        {ImageFormat::RGBA8, ImageFormatInternal::Compressed_RGBA_ASTC_8x8, ImagePixelType::UByte, 4},

        {ImageFormat::RGB8,  ImageFormatInternal::Compressed_RGB_DXT1, ImagePixelType::UByte, 3},
        {ImageFormat::RGBA8, ImageFormatInternal::Compressed_RGBA_DXT5, ImagePixelType::UByte, 4},
    }};
    return ImageFormatInfoMap[static_cast<int>(fmt)];
}

ImageFormat Tx_GetImageFormat(int nrChannels, int bits, bool isHDR) {
    if (isHDR && bits == 32) {
        switch (nrChannels) {
            case 1: return ImageFormat::R32F;
            case 2: return ImageFormat::RG32F;
            case 3: return ImageFormat::RGB32F;
            case 4: return ImageFormat::RGBA32F;
        }
    } else {
        if (bits == 8) {
            switch (nrChannels) {
                case 1: return ImageFormat::R8;
                case 2: return ImageFormat::RG8;
                case 3: return ImageFormat::RGB8;
                case 4: return ImageFormat::RGBA8;
            }
        } else if (bits == 16) {
            switch (nrChannels) {
                case 1: return ImageFormat::R16;
                case 2: return ImageFormat::RG16;
                case 3: return ImageFormat::RGB16;
                case 4: return ImageFormat::RGBA16;
            }
        }
    }
    throw std::runtime_error("ImageFormat not found for nrChannels="
        + std::to_string(nrChannels) + ", bits="
        + std::to_string(bits) + ", isHDR="
        + std::to_string(isHDR)
    );
}

}