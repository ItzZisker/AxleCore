#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/data/AX_DataStreamImplFile.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_LittleEndian.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <ostream>

namespace axle::audio {

WAVAudio WAV_LoadFileBytes(data::DataDeserializer *buffer) {
    if (!WAV_isValidFileBytes(buffer)) {
        std::cout << "FF\n";
        throw std::runtime_error("Invalid WAV");
    }

    std::cout << "readpos=" << buffer->GetReadPos() << std::endl;

        std::cout << "A\n";
    WAVAudio wav;
    buffer->Read(wav.header.riff, 4);
    buffer->Skip(4);
    buffer->Read(wav.header.wave, 4);
    buffer->Read(wav.header.fmt, 4);
    buffer->Skip(4);
    wav.header.audioFormat = data::LE_Read<uint16_t>(buffer);
    wav.header.numChannels = data::LE_Read<uint16_t>(buffer);
    wav.header.sampleRate = data::LE_Read<uint32_t>(buffer);
    wav.header.byteRate = data::LE_Read<uint32_t>(buffer);
    wav.header.blockAlign = data::LE_Read<uint16_t>(buffer);
    wav.header.bitsPerSample = data::LE_Read<uint16_t>(buffer);

    std::cout << "wav.header.audioFormat=" << wav.header.audioFormat << std::endl;
    std::cout << "wav.header.numChannels=" << wav.header.numChannels << std::endl;
    std::cout << "wav.header.sampleRate=" << wav.header.sampleRate << std::endl;
    std::cout << "wav.header.byteRate=" << wav.header.byteRate << std::endl;
    std::cout << "wav.header.blockAlign=" << wav.header.blockAlign << std::endl;
    std::cout << "wav.header.bitsPerSample=" << wav.header.bitsPerSample << std::endl;

    char dataHeader[4];
    uint32_t dataSize;
    do {
        std::cout << "B\n";
        buffer->Read(dataHeader, 4);
        dataSize = data::LE_Read<uint32_t>(buffer);
        std::cout << "dataHeader=" << std::string(dataHeader) << std::endl;
        if(std::string(dataHeader, 4) != "data")
            buffer->Skip(dataSize);
    } while (std::string(dataHeader,4) != "data");

    wav.format = (wav.header.numChannels == 1 ?
                 (wav.header.bitsPerSample == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) :
                 (wav.header.bitsPerSample == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16));
    wav.samples.resize(dataSize);
    buffer->Read((uint8_t*) wav.samples.data(), dataSize);

    return wav;
}

WAVAudio WAV_LoadFileBytes(const uint8_t* bytes, int length) {
    std::shared_ptr<data::BufferDataStream> stream = std::make_shared<data::BufferDataStream>(bytes, length);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
    WAVAudio wav = WAV_LoadFileBytes(buffer);
    delete buffer;
    return wav;
}

WAVAudio WAV_LoadFile(const std::filesystem::path& path) {
        std::cout << "a\n";
    std::shared_ptr<data::FileDataStream> stream = std::make_shared<data::FileDataStream>(path, true, false);
    data::DataDeserializer *buffer = new data::DataDeserializer(stream);
        std::cout << "b\n";
    WAVAudio wav = WAV_LoadFileBytes(buffer);
        std::cout << "c\n";
    delete buffer;
    return wav;
}

bool WAV_isValidFileBytes(data::DataDeserializer *buffer) {
    if (buffer->GetLength() < 12) return false;
    char riff[4], wave[4];
    buffer->Read((uint8_t*) riff, 4);
    buffer->Skip(4);
    buffer->Read((uint8_t*) wave, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0 ||
        std::strncmp(wave, "WAVE", 4) != 0) {
        buffer->Rewind(12);
        return false;
    }
    buffer->Rewind(12);
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