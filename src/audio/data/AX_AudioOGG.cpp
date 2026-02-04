#include "axle/audio/data/AX_AudioOGG.hpp"

#include "axle/data/AX_DataStreamImplFile.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <cstring>
#include <filesystem>
#include <memory>

namespace axle::audio {

OGGAudio OGG_LoadFileBytes(data::DataDeserializer& buffer) {
    int channels, sampleRate;
    
    size_t len = buffer.GetLength();
    std::vector<uint8_t> fileData(len);
    
    size_t originalPos = buffer.GetReadPos();
    buffer.Rewind(originalPos);
    buffer.Read(fileData.data(), len);
    buffer.Rewind(len); // Restore pos

    int error;
    stb_vorbis_info* vor = (stb_vorbis_info*) stb_vorbis_open_memory(fileData.data(), (int)fileData.size(), &error, nullptr);

    OGGAudio ogg;
    ogg.channels = vor->channels;
    ogg.sampleRate = vor->sample_rate;
    ogg.entireStream = fileData;

    stb_vorbis_close((stb_vorbis*)vor);
    return ogg;
}

OGGAudio OGG_LoadFileBytes(const uint8_t* bytes, int length) {
    auto stream = std::make_shared<data::BufferDataStream>(bytes, length);
    data::DataDeserializer buffer(stream);
    return OGG_LoadFileBytes(buffer);
}

OGGAudio OGG_LoadFile(const std::filesystem::path& path) {
    auto stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer buffer(stream);
    return OGG_LoadFileBytes(buffer);
}

bool OGG_isValidFileBytes(data::DataDeserializer& buffer) {
    if (buffer.GetLength() < 4) return false;
    
    char capture[4];
    buffer.Read(capture, 4);
    bool valid = (std::strncmp(capture, "OggS", 4) == 0);
    
    buffer.Rewind(4);
    return valid;
}

bool OGG_isValidFileBytes(const uint8_t* bytes, int length) {
    if (length < 4) return false;
    return std::strncmp((const char*)bytes, "OggS", 4) == 0;
}

bool OGG_isValidFile(const std::filesystem::path& path) {
    auto stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer buffer(stream);
    return OGG_isValidFileBytes(buffer);
}

}