#include "axle/graphics/base/image/AX_ImageASTC.hpp"

#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_DataStreamImplFile.hpp"

namespace axle::gfx {

utils::ExResult<ASTCImage> ASTC_LoadFileBytes(data::IDataStream& buffer) {
    ASTCImage img{};
    ASTCHeader header;
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(reinterpret_cast<uint8_t*>(&header), sizeof(header)));

    if (header.magic[0] != 0x13 ||
        header.magic[1] != 0xAB ||
        header.magic[2] != 0xA1 ||
        header.magic[3] != 0x5C) {
        return utils::ExError("Not a valid ASTC file format");
    }

    img.blockX = header.blockdim_x;
    img.blockY = header.blockdim_y;
    img.blockZ = header.blockdim_z;

    img.width  = header.xsize[0] + (header.xsize[1] << 8) + (header.xsize[2] << 16);
    img.height = header.ysize[0] + (header.ysize[1] << 8) + (header.ysize[2] << 16);
    img.depth  = header.zsize[0] + (header.zsize[1] << 8) + (header.zsize[2] << 16);

    // Read compressed data
    size_t dataSize = buffer.GetLength() - sizeof(ASTCHeader);

    std::vector<uint8_t> buff;
    buff.resize(dataSize);

    AX_PROPAGATE_RESULT_ERROR(buffer.Read(buff.data(), dataSize));
    img.data = {std::move(buff)};

    return img;
}

utils::ExResult<ASTCImage> ASTC_LoadFileBytes(utils::Span<uint8_t> bufferView) {
    std::shared_ptr<data::BufferDataStream> stream = std::make_shared<data::BufferDataStream>(bufferView);
    AX_PROPAGATE_ERROR(stream->Open());
    return ASTC_LoadFileBytes(*stream);
}

utils::ExResult<ASTCImage> ASTC_LoadFile(const std::filesystem::path& path) {
    std::shared_ptr<data::FileDataStream> stream = std::make_shared<data::FileDataStream>(path, true, false);
    AX_PROPAGATE_ERROR(stream->Open());
    return ASTC_LoadFileBytes(*stream);
}

}