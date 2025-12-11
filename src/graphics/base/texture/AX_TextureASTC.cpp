#include "axle/graphics/base/texture/TextureASTC.hpp"

ASTCImage loadASTC_FileBytes(DataDeserializer *buffer) {
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
    buffer->read(reinterpret_cast<uint8_t*>(&header), sizeof(header));

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
    size_t dataSize = buffer->getLength() - sizeof(ASTCHeader);
    img.data.resize(dataSize);
    buffer->read(img.data.data(), dataSize);

    return img;
}

ASTCImage loadASTC_FileBytes(const uint8_t* bytes, int length) {
    std::shared_ptr<BufferDataStream> stream = std::make_shared<BufferDataStream>(bytes, length);
    DataDeserializer *buffer = new DataDeserializer(stream);
    ASTCImage img = loadASTC_FileBytes(buffer);
    delete buffer;
    return img;
}

ASTCImage loadASTC_File(const std::filesystem::path& path) {
    std::shared_ptr<FileDataStream> stream = std::make_shared<FileDataStream>(path, true, false);
    DataDeserializer *buffer = new DataDeserializer(stream);
    ASTCImage img = loadASTC_FileBytes(buffer);
    delete buffer;
    return img;
}

bool isASTC_FileBytes(DataDeserializer *buffer) {
    uint8_t magic[4];
    buffer->read(magic, 4);
    return magic[0] == 0x13 && magic[1] == 0xAB && magic[2] == 0xA1 && magic[3] == 0x5C;
}

bool isASTC_FileBytes(const uint8_t* bytes, int length) {
    if (length < 4 + 20) return false;
    std::shared_ptr<BufferDataStream> stream = std::make_shared<BufferDataStream>(bytes, length);
    DataDeserializer *buffer = new DataDeserializer(stream);
    bool result = isASTC_FileBytes(buffer);
    delete buffer;
    return result;
}

bool isASTC_File(const std::filesystem::path& path) {
    std::shared_ptr<FileDataStream> stream = std::make_shared<FileDataStream>(path, true, false);
    DataDeserializer *buffer = new DataDeserializer(stream);
    if (buffer->getLength() < 4 + 20) return false;
    bool result = isASTC_FileBytes(buffer);
    delete buffer;
    return result;
}