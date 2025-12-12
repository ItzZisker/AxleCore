#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/data/AX_DataStreamImplFile.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include <cstring>
#include <stdexcept>

namespace axle::audio {

WAVAudio WAV_LoadFileBytes(data::DataDeserializer *buffer) {
    WAVAudio res;
    WAVHeader& header = res.header;

    buffer->Read(reinterpret_cast<char *>(&header), sizeof(WAVHeader));
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        throw std::runtime_error("Invalid WAV, riff=" + std::string(header.riff) + ", wave=" + std::string(header.wave));
    }

    if (header.numChannels == 1 && header.bitsPerSample == 8) res.format = AL_FORMAT_MONO8;
    else if (header.numChannels == 1 && header.bitsPerSample == 16) res.format = AL_FORMAT_MONO16;
    else if (header.numChannels == 2 && header.bitsPerSample == 8) res.format = AL_FORMAT_STEREO8;
    else if (header.numChannels == 2 && header.bitsPerSample == 16) res.format = AL_FORMAT_STEREO16;
    else throw std::runtime_error("Invalid WAV Format, numChannels=" 
        + std::to_string(header.numChannels) + ", bitsPerSample=" 
        + std::to_string(header.bitsPerSample)
    );

    res.samples.resize(header.dataSize);
    buffer->Read(res.samples.data(), header.dataSize);

    return res;
}

WAVAudio WAV_LoadFileBytes(const uint8_t* bytes, int length) {
    std::shared_ptr<data::BufferDataStream> stream = std::make_shared<data::BufferDataStream>(bytes, length);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    WAVAudio wav = WAV_LoadFileBytes(buffer);
    delete buffer;
    return wav;
}

WAVAudio WAV_LoadFile(const std::filesystem::path& path) {
    std::shared_ptr<data::FileDataStream> stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    WAVAudio wav = WAV_LoadFileBytes(buffer);
    delete buffer;
    return wav;
}

bool WAV_isValidFileBytes(data::DataDeserializer *buffer) {
    if (buffer->GetLength() < sizeof(WAVHeader)) return false;
    WAVHeader header;
    buffer->Read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        return false;
    }
    return true;
}

bool WAV_isValidFileBytes(const uint8_t* bytes, int length) {
    if (length < sizeof(WAVHeader)) return false;
    std::shared_ptr<data::BufferDataStream> stream = std::make_shared<data::BufferDataStream>(bytes, length);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    bool result = WAV_isValidFileBytes(buffer);
    delete buffer;
    return result;
}

bool WAV_isValidFile(const std::filesystem::path& path) {
    std::shared_ptr<data::FileDataStream> stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    if (buffer->GetLength() < 4 + 20) return false;
    bool result = WAV_isValidFileBytes(buffer);
    delete buffer;
    return result;
}

}