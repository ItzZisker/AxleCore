#include "axle/graphics/AX_GraphicsParams.hpp"

#include "axle/utils/AX_Universal.hpp"

using namespace axle::utils;

namespace axle::gfx
{

VertexSemantic ParseSemantic(const char* name) {
    if (name == nullptr)
        return VertexSemantic::Custom;

    if (EqualsIgnoreCase(name, "POSITION"))
        return VertexSemantic::Position;
    if (EqualsIgnoreCase(name, "NORMAL"))
        return VertexSemantic::Normal;
    if (EqualsIgnoreCase(name, "TEXCOORD"))
        return VertexSemantic::TexCoord;
    if (EqualsIgnoreCase(name, "TANGENT"))
        return VertexSemantic::Tangent;
    if (EqualsIgnoreCase(name, "BINORMAL"))
        return VertexSemantic::Bitangent;
    if (EqualsIgnoreCase(name, "BITANGENT"))
        return VertexSemantic::Bitangent;
    if (EqualsIgnoreCase(name, "COLOR"))
        return VertexSemantic::Color;
    if (EqualsIgnoreCase(name, "BLENDINDICES"))
        return VertexSemantic::BoneIndices;
    if (EqualsIgnoreCase(name, "BLENDWEIGHT"))
        return VertexSemantic::BoneWeights;

    return VertexSemantic::Custom;
}

uint32_t CalcImageSize(const TextureImageDescriptor& desc) {
    uint32_t w = std::max(1u, desc.width  >> desc.mip);
    uint32_t h = std::max(1u, desc.height >> desc.mip);
    uint32_t d{1};

    if (desc.type == gfx::TextureType::Texture3D) {
        d = std::max(1u, desc.depth >> desc.mip);
    } else if (desc.type == gfx::TextureType::Array2D) {
        d = desc.layers;
    }

    switch (desc.format) {
        // BC1 / DXT1 (8 bytes per 4x4 block)
        case TextureFormat::BC1_UNORM: {
            uint32_t bw = (w + 3) / 4;
            uint32_t bh = (h + 3) / 4;
            return bw * bh * 8 * d;
        }
        // BC2 / BC3 (16 bytes per 4x4 block)
        case TextureFormat::BC3_UNORM: {
            uint32_t bw = (w + 3) / 4;
            uint32_t bh = (h + 3) / 4;
            return bw * bh * 16 * d;
        }
        // BC4
        case TextureFormat::BC4_UNORM: {
            uint32_t bw = (w + 3) / 4;
            uint32_t bh = (h + 3) / 4;
            return bw * bh * 8 * d;
        }
        // BC5
        case TextureFormat::BC5_UNORM: {
            uint32_t bw = (w + 3) / 4;
            uint32_t bh = (h + 3) / 4;
            return bw * bh * 16 * d;
        }
        // ASTC formats (always 16 bytes per block)
        case TextureFormat::ASTC_4x4_UNORM:
        case TextureFormat::ASTC_4x4_SRGB: {
            uint32_t bw = (w + 3) / 4;
            uint32_t bh = (h + 3) / 4;
            return bw * bh * 16 * d;
        }
        case TextureFormat::ASTC_6x6_UNORM:
        case TextureFormat::ASTC_6x6_SRGB: {
            uint32_t bw = (w + 5) / 6;
            uint32_t bh = (h + 5) / 6;
            return bw * bh * 16 * d;
        }
        case TextureFormat::ASTC_8x8_UNORM:
        case TextureFormat::ASTC_8x8_SRGB: {
            uint32_t bw = (w + 7) / 8;
            uint32_t bh = (h + 7) / 8;
            return bw * bh * 16 * d;
        }
        default: return 0;
    }
}

}