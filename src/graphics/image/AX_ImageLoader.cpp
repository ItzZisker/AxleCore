#include "axle/graphics/image/AX_ImageLoader.hpp"

#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_DataStreamImplFile.hpp"

#include "axle/utils/AX_Universal.hpp"

#include "stb_image.h"

#include <memory>
#include <vector>
#include <iostream>

using namespace axle::utils;
using namespace axle::data;

namespace axle::gfx
{

struct STBI_SharedUserPtr {
    IDataStream* buffer = nullptr;
    std::vector<ExError> errors;
};

int STBI_Read(void* user_, char* data, int size) {
    auto* user = static_cast<STBI_SharedUserPtr*>(user_);
    auto res = user->buffer->Read(data, size);
    if (res.has_value()) {
        return res.value();
    } else {
        user->errors.push_back(res.error());
        return 0;
    }
}

void STBI_Skip(void* user_, int n) {
    auto* user = static_cast<STBI_SharedUserPtr*>(user_);
    auto err = user->buffer->SkipRead(n);
    if (err.IsValid()) {
        user->errors.push_back(err);
    }
}

int STBI_EOF(void* user_) {
    auto* user = static_cast<STBI_SharedUserPtr*>(user_);
    if (!user->errors.empty()) {
        return 1;
    }
    return user->buffer->EndOfStream() ? 1 : 0;
}

bool Img_ASTC_IsValidFileBytes(IDataStream& buffer) {
    if (buffer.GetReadIndex() + 4 >= buffer.GetLength())
        return false;
    auto prevIdx = buffer.GetReadIndex();
    uint8_t magic[4]{0, 0, 0, 0};
    buffer.Read(magic, 4);
    buffer.SeekRead(prevIdx);
    return magic[0] == 0x13 && magic[1] == 0xAB && magic[2] == 0xA1 && magic[3] == 0x5C;
}

bool Img_ASTC_IsValidFileBytes(URawView bufferView) {
    auto buffer = BufferDataStream(bufferView);
    if (buffer.Open().IsValid()) return false;
    return Img_ASTC_IsValidFileBytes(buffer);
}

bool Img_ASTC_IsValidFile(const std::filesystem::path& path) {
    auto buffer = FileDataStream(path, true, false);
    if (buffer.Open().IsValid()) return false;
    return Img_ASTC_IsValidFileBytes(buffer);
}

ExResult<ASTCImage> Img_ASTC_LoadFileBytes(IDataStream& buffer) {
    if (!Img_ASTC_IsValidFileBytes(buffer)) {
        return ExError("Invalid ASTC Bytes");
    }

    typedef struct {
        uint8_t magic[4];       // 0x13, 0xAB, 0xA1, 0x5C
        uint8_t blockdim_x;     // usually 4–12
        uint8_t blockdim_y;     // usually 4–12
        uint8_t blockdim_z;     // usually 1
        uint8_t xsize[3];       // little-endian 24-bit
        uint8_t ysize[3];
        uint8_t zsize[3];
    } ASTCHeader;

    ASTCImage img{};
    ASTCHeader header;
    header.magic[0] = 0;
    header.magic[1] = 0;
    header.magic[2] = 0;
    header.magic[3] = 0;

    AX_PROPAGATE_RESULT_ERROR(buffer.Read((uint8_t*)&header, sizeof(header)));
    if (header.magic[0] != 0x13 ||
        header.magic[1] != 0xAB ||
        header.magic[2] != 0xA1 ||
        header.magic[3] != 0x5C
    ) {
        return ExError("Not a valid ASTC file (header magic mismatch)");
    }

    img.blockX = header.blockdim_x;
    img.blockY = header.blockdim_y;
    img.blockZ = header.blockdim_z;

    img.width  = header.xsize[0] + (header.xsize[1] << 8) + (header.xsize[2] << 16);
    img.height = header.ysize[0] + (header.ysize[1] << 8) + (header.ysize[2] << 16);
    img.depth  = header.zsize[0] + (header.zsize[1] << 8) + (header.zsize[2] << 16);

    size_t dataSize = buffer.GetLength() - sizeof(ASTCHeader);
    img.data = utils::URaw(std::vector<uint8_t>(dataSize));
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(img.data.data(), dataSize));

    return img;
}

ExResult<ASTCImage> Img_ASTC_LoadFileBytes(URawView bufferView) {
    if (!Img_ASTC_IsValidFileBytes(bufferView)) {
        return ExError("Invalid ASTC Bytes");
    }
    auto buffer = BufferDataStream(bufferView);
    AX_PROPAGATE_ERROR(buffer.Open());
    return Img_ASTC_LoadFileBytes(buffer);
}

ExResult<ASTCImage> Img_ASTC_LoadFile(const std::filesystem::path& path) {
    if (!Img_ASTC_IsValidFile(path)) {
        return ExError("Invalid ASTC File");
    }
    auto buffer = FileDataStream(path, true, false);
    AX_PROPAGATE_ERROR(buffer.Open());
    return Img_ASTC_LoadFileBytes(buffer);
}

bool Img_BCn_IsValidFileBytes(IDataStream& buffer) {
    if (buffer.GetReadIndex() + 4 >= buffer.GetLength()){
        return false;
    }
    auto prevIdx = buffer.GetReadIndex();
    uint8_t magic[4]{0, 0, 0, 0};
    buffer.Read(magic, 4);
    buffer.SeekRead(prevIdx);
    return magic[0] == 'D' && magic[1] == 'X' && magic[2] == 'T' && magic[3] == '\0';
}

bool Img_BCn_IsValidFileBytes(URawView bufferView) {
    auto buffer = BufferDataStream(bufferView);
    if (buffer.Open().IsValid()) return false;
    return Img_BCn_IsValidFileBytes(buffer);
}

bool Img_BCn_IsValidFile(const std::filesystem::path& path) {
    auto buffer = FileDataStream(path, true, false);
    if (buffer.Open().IsValid()) return false;
    return Img_BCn_IsValidFileBytes(buffer);
}

utils::ExResult<BCnImage> Img_BCn_LoadFileBytes(data::IDataStream& buffer) {
    if (!Img_BCn_IsValidFileBytes(buffer)) {
        return ExError("Invalid BCn/DXT Bytes");
    }

    typedef struct {
        uint8_t magic[4];   // "DXT\0"
        uint16_t width;
        uint16_t height;
        uint8_t alpha;      // 0 = BC1/DXT1, 1 = BC3/DXT5
        uint8_t reserved;   // alignment padding
    } BCnHeader;

    BCnImage img{};
    BCnHeader header;
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(reinterpret_cast<uint8_t*>(&header), sizeof(header)));

    if (header.magic[0] != 'D' ||
        header.magic[1] != 'X' ||
        header.magic[2] != 'T' ||
        header.magic[3] != '\0'
    ) {
        return ExError("Not a valid DXT file (header magic missmatch)");
    }

    img.width = header.width;
    img.height = header.height;
    img.alpha = header.alpha;

    size_t dataSize = buffer.GetLength() - sizeof(BCnHeader);
    std::vector<uint8_t> data(dataSize);
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(data.data(), dataSize));
    img.data = std::move(data);

    return img;
}

utils::ExResult<BCnImage> Img_BCn_LoadFileBytes(utils::URawView bufferView) {
    if (!Img_BCn_IsValidFileBytes(bufferView)) {
        return ExError("Invalid BCn/DXT Bytes");
    }
    auto buffer = BufferDataStream(bufferView);
    AX_PROPAGATE_ERROR(buffer.Open());
    return Img_BCn_LoadFileBytes(buffer);
}

utils::ExResult<BCnImage> Img_BCn_LoadFile(const std::filesystem::path& path) {
    if (!Img_BCn_IsValidFile(path)) {
        return ExError("Invalid BCn/DXT File");
    }
    auto buffer = FileDataStream(path, true, false);
    AX_PROPAGATE_ERROR(buffer.Open());
    return Img_BCn_LoadFileBytes(buffer);
}

utils::ExResult<Image> Img_Auto_LoadFileBytes(IDataStream& buffer) {
    Image img;

    if (Img_ASTC_IsValidFileBytes(buffer)) {
        AX_DECL_OR_PROPAGATE(astc, Img_ASTC_LoadFileBytes(buffer));
        if (astc.blockX == 4 && astc.blockY == 4) img.format = ImageFormat::Compressed_RGBA_ASTC_4x4;
        else if (astc.blockX == 5 && astc.blockY == 5) img.format = ImageFormat::Compressed_RGBA_ASTC_5x5;
        else if (astc.blockX == 6 && astc.blockY == 6) img.format = ImageFormat::Compressed_RGBA_ASTC_6x6;
        else if (astc.blockX == 8 && astc.blockY == 8) img.format = ImageFormat::Compressed_RGBA_ASTC_8x8;
        else return ExError("Unsupported ASTC Image (block dims)");
        img.bytes = std::move(astc.data);
        img.width = astc.width;
        img.height = astc.height;
        return img;
    } else if (Img_BCn_IsValidFileBytes(buffer)) {
        AX_DECL_OR_PROPAGATE(bcn, Img_BCn_LoadFileBytes(buffer));
        img.format = bcn.alpha ? ImageFormat::Compressed_RGBA_DXT5 : ImageFormat::Compressed_RGB_DXT1;
        img.bytes = std::move(bcn.data);
        img.width = bcn.width;
        img.height = bcn.height;
        return img;
    }

    const static thread_local stbi_io_callbacks clbks {
        .read = &STBI_Read,
        .skip = &STBI_Skip,
        .eof  = &STBI_EOF
    };

    STBI_SharedUserPtr stbi_user {
        .buffer = &buffer,
        .errors = {}
    };

    auto prevIdx = buffer.GetReadIndex();
    ScopeCall reset_stbi([&stbi_user, prevIdx](){
        stbi_user.buffer->SeekRead(prevIdx);
        stbi_user.errors.clear();
    });

    reset_stbi.Call();
    if (stbi_is_hdr_from_callbacks(&clbks, &stbi_user)) {
        reset_stbi.Call();
        int channels;
        void* data = stbi_loadf_from_callbacks(&clbks, &stbi_user, &img.width, &img.height, &channels, 0);
        if (!data) {
            if (!stbi_user.errors.empty()) return stbi_user.errors.front();
            return ExError(stbi_failure_reason() ? stbi_failure_reason() : "Failed to load HDR image");
        }
        switch (channels) {
            case 1: img.format = ImageFormat::Raw_R32F;     break;
            case 2: img.format = ImageFormat::Raw_RG32F;    break;
            case 3: img.format = ImageFormat::Raw_RGB32F;   break;
            case 4: img.format = ImageFormat::Raw_RGBA32F;  break;
            default: {
                stbi_image_free(data);
                return ExError("Failed to load HDR Image: Invalid Channel count");
            }
        }
        std::vector<uint8_t> bytes(img.GetSize());
        std::memcpy(bytes.data(), data, img.GetSize());
        img.bytes = URaw(std::move(bytes));
        stbi_image_free(data);
        return img;
    } 

    reset_stbi.Call();
    if (stbi_is_16_bit_from_callbacks(&clbks, &stbi_user)) {
        reset_stbi.Call();
        int channels;
        void* data = stbi_load_16_from_callbacks(&clbks, &stbi_user, &img.width, &img.height, &channels, 0);
        if (!data) {
            if (!stbi_user.errors.empty()) return stbi_user.errors.front();
            return ExError(stbi_failure_reason() ? stbi_failure_reason() : "Failed to load 16-bit image");
        }
        switch (channels) {
            case 1: img.format = ImageFormat::Raw_R16;      break;
            case 2: img.format = ImageFormat::Raw_RG16;     break;
            case 3: img.format = ImageFormat::Raw_RGB16;    break;
            case 4: img.format = ImageFormat::Raw_RGBA16;   break;
            default: {
                stbi_image_free(data);
                return ExError("Failed to load 16-bit Image: Invalid Channel count");
            }
        }
        std::vector<uint8_t> bytes(img.GetSize());
        std::memcpy(bytes.data(), data, img.GetSize());
        img.bytes = URaw(std::move(bytes));
        stbi_image_free(data);
        return img;
    }

    int channels;
    reset_stbi.Call();
    void* data = stbi_load_from_callbacks(&clbks, &stbi_user, &img.width, &img.height, &channels, 0);
    if (!data) {
        if (!stbi_user.errors.empty()) return stbi_user.errors.front();
        return ExError(stbi_failure_reason() ? stbi_failure_reason() : "Failed to load 8-bit image");
    }
    switch (channels) {
        case 1: img.format = ImageFormat::Raw_R8;       break;
        case 2: img.format = ImageFormat::Raw_RG8;      break;
        case 3: img.format = ImageFormat::Raw_RGB8;     break;
        case 4: img.format = ImageFormat::Raw_RGBA8;    break;
        default: {
            stbi_image_free(data);
            return ExError("Failed to load 8-bit Image: Invalid Channel count");
        }
    }
    std::vector<uint8_t> bytes(img.GetSize());
    std::memcpy(bytes.data(), data, img.GetSize());
    img.bytes = URaw(std::move(bytes));
    stbi_image_free(data);
    return img;
}

utils::ExResult<Image> Img_Auto_LoadFileBytes(URawView bufferView) {
    auto buffer = BufferDataStream(bufferView);
    AX_PROPAGATE_ERROR(buffer.Open());
    return Img_Auto_LoadFileBytes(buffer);
}

utils::ExResult<Image> Img_Auto_LoadFile(const std::filesystem::path& path) {
    auto buffer = FileDataStream(path, true, false);
    AX_PROPAGATE_ERROR(buffer.Open());
    return Img_Auto_LoadFileBytes(buffer);
}

bool Img_IsCompressed(const ImageFormat& fmt) {
    switch (fmt) {
        case ImageFormat::Raw_R8:
        case ImageFormat::Raw_RG8:
        case ImageFormat::Raw_RGB8:
        case ImageFormat::Raw_RGBA8:
        case ImageFormat::Raw_R16:
        case ImageFormat::Raw_RG16:
        case ImageFormat::Raw_RGB16:
        case ImageFormat::Raw_RGBA16:
        case ImageFormat::Raw_R32F:
        case ImageFormat::Raw_RG32F:
        case ImageFormat::Raw_RGB32F:
        case ImageFormat::Raw_RGBA32F:
            return false;
        case ImageFormat::Compressed_RGBA_ASTC_4x4:
        case ImageFormat::Compressed_RGBA_ASTC_5x5:
        case ImageFormat::Compressed_RGBA_ASTC_6x6:
        case ImageFormat::Compressed_RGBA_ASTC_8x8:
        case ImageFormat::Compressed_RGBA_DXT5:
        case ImageFormat::Compressed_RGB_DXT1:
            return true;
    }
    return false;
}

uint32_t Img_GetChannelCount(const ImageFormat& fmt) {
    switch (fmt) {
        case ImageFormat::Raw_R8:       return 1;
        case ImageFormat::Raw_RG8:      return 2;
        case ImageFormat::Raw_RGB8:     return 3;
        case ImageFormat::Raw_RGBA8:    return 4;

        case ImageFormat::Raw_R16:      return 1;
        case ImageFormat::Raw_RG16:     return 2;
        case ImageFormat::Raw_RGB16:    return 3;
        case ImageFormat::Raw_RGBA16:   return 4;

        case ImageFormat::Raw_R32F:     return 1;
        case ImageFormat::Raw_RG32F:    return 2;
        case ImageFormat::Raw_RGB32F:   return 3;
        case ImageFormat::Raw_RGBA32F:  return 4;

        case ImageFormat::Compressed_RGBA_ASTC_4x4:
        case ImageFormat::Compressed_RGBA_ASTC_5x5:
        case ImageFormat::Compressed_RGBA_ASTC_6x6:
        case ImageFormat::Compressed_RGBA_ASTC_8x8: return 4;

        case ImageFormat::Compressed_RGBA_DXT5: return 4;
        case ImageFormat::Compressed_RGB_DXT1:  return 3;
    }
    return 0;
}

uint32_t Img_GetBytesPerChannel(const ImageFormat& fmt) {
    switch (fmt) {
        case ImageFormat::Raw_R8:
        case ImageFormat::Raw_RG8:
        case ImageFormat::Raw_RGB8:
        case ImageFormat::Raw_RGBA8:    return 1;
        case ImageFormat::Raw_R16:
        case ImageFormat::Raw_RG16:
        case ImageFormat::Raw_RGB16:
        case ImageFormat::Raw_RGBA16:   return 2;
        case ImageFormat::Raw_R32F:
        case ImageFormat::Raw_RG32F:
        case ImageFormat::Raw_RGB32F:
        case ImageFormat::Raw_RGBA32F:  return 4;
        case ImageFormat::Compressed_RGBA_ASTC_4x4:
        case ImageFormat::Compressed_RGBA_ASTC_5x5:
        case ImageFormat::Compressed_RGBA_ASTC_6x6:
        case ImageFormat::Compressed_RGBA_ASTC_8x8:
        case ImageFormat::Compressed_RGBA_DXT5:
        case ImageFormat::Compressed_RGB_DXT1:  return 0; // ???
    }
    return 0;
}

}