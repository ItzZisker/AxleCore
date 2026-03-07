#include "axle/audio/data/AX_AudioOGG.hpp"

#include "axle/data/AX_DataStreamImplFile.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <filesystem>
#include <cstring>
#include <memory>

namespace axle::audio {

utils::ExResult<OGGAudio> OGG_LoadFileBytes(data::IDataStream& buffer) {
    size_t len = buffer.GetLength();

    if (len < 4) {
        return utils::ExError{"Invalid Ogg file format"};
    }
    std::vector<uint8_t> fileData(len);

    char magic[4];
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(magic, 4));
    if (std::strncmp(magic, "OggS", 4) != 0) {
        return utils::ExError{"Invalid Ogg file format"};;
    }

    size_t originalPos = buffer.GetReadIndex();
    AX_PROPAGATE_ERROR(buffer.SeekRead(0));
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(fileData.data(), len));
    AX_PROPAGATE_ERROR(buffer.SeekRead(originalPos)); // Restore pos

    int error;
    stb_vorbis_info* vor = (stb_vorbis_info*) stb_vorbis_open_memory(fileData.data(), (int)fileData.size(), &error, nullptr);
    if (!vor || error) {
        return utils::ExError{"Cannot open OGGAudio: stb_vorbis_open_memory failed: error=" + std::to_string(error)};
    }

    OGGAudio ogg;
    ogg.channels = vor->channels;
    ogg.sampleRate = vor->sample_rate;
    ogg.entireStream = fileData;

    stb_vorbis_close((stb_vorbis*)vor);
    return ogg;
}

utils::ExResult<OGGAudio> OGG_LoadFileBytes(utils::Span<uint8_t> bufferView) {
    auto stream = std::make_shared<data::BufferDataStream>(bufferView);
    AX_PROPAGATE_ERROR(stream->Open());
    return OGG_LoadFileBytes(*stream);
}

utils::ExResult<OGGAudio> OGG_LoadFile(const std::filesystem::path& path) {
    auto stream = std::make_shared<data::FileDataStream>(path, true, false);
    AX_PROPAGATE_ERROR(stream->Open());
    return OGG_LoadFileBytes(*stream);
}

}