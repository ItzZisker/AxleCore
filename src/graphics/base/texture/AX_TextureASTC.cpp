#include "axle/graphics/base/texture/AX_TextureASTC.hpp"

#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_DataStreamImplFile.hpp"

namespace axle::graphics {

ASTCImage ASTC_LoadFileBytes(data::DataDeserializer *buffer) {
    ASTCImage img{};
    ASTCHeader header;
    buffer->Read(reinterpret_cast<uint8_t*>(&header), sizeof(header));

    if (header.magic[0] != 0x13 || header.magic[1] != 0xAB || header.magic[2] != 0xA1 || header.magic[3] != 0x5C) {
        throw std::runtime_error("Not a valid ASTC file");
    }

    img.blockX = header.blockdim_x;
    img.blockY = header.blockdim_y;
    img.blockZ = header.blockdim_z;

    img.width  = header.xsize[0] + (header.xsize[1] << 8) + (header.xsize[2] << 16);
    img.height = header.ysize[0] + (header.ysize[1] << 8) + (header.ysize[2] << 16);
    img.depth  = header.zsize[0] + (header.zsize[1] << 8) + (header.zsize[2] << 16);

    // Read compressed data
    size_t dataSize = buffer->GetLength() - sizeof(ASTCHeader);
    img.data.resize(dataSize);
    buffer->Read(img.data.data(), dataSize);

    return img;
}

ASTCImage ASTC_LoadFileBytes(const uint8_t* bytes, int length) {
    std::shared_ptr<data::BufferDataStream> stream = std::make_shared<data::BufferDataStream>(bytes, length);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    ASTCImage img = ASTC_LoadFileBytes(buffer);
    delete buffer;
    return img;
}

ASTCImage ASTC_LoadFile(const std::filesystem::path& path) {
    std::shared_ptr<data::FileDataStream> stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    ASTCImage img = ASTC_LoadFileBytes(buffer);
    delete buffer;
    return img;
}

bool ASTC_isValidFileBytes(data::DataDeserializer *buffer) {
    uint8_t magic[4];
    buffer->Read(magic, 4);
    return magic[0] == 0x13 && magic[1] == 0xAB && magic[2] == 0xA1 && magic[3] == 0x5C;
}

bool ASTC_isValidFileBytes(const uint8_t* bytes, int length) {
    if (length < 4 + 20) return false;
    std::shared_ptr<data::BufferDataStream> stream = std::make_shared<data::BufferDataStream>(bytes, length);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    bool result = ASTC_isValidFileBytes(buffer);
    delete buffer;
    return result;
}

bool ASTC_isValidFile(const std::filesystem::path& path) {
    std::shared_ptr<data::FileDataStream> stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    if (buffer->GetLength() < 4 + 20) return false;
    bool result = ASTC_isValidFileBytes(buffer);
    delete buffer;
    return result;
}

}