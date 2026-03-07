#include "axle/audio/data/AX_AudioWAV.hpp"

#include "axle/data/AX_DataStreamImplFile.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_DataEndianness.hpp"

#include <cstdint>
#include <cstring>

namespace axle::audio {

utils::ExResult<WAVAudio> WAV_LoadFileBytes(data::IDataStream& buffer) {
    if (buffer.GetLength() < 12) {
        return utils::ExError{"Invalid WAV file format"};
    }

    char riff[4], wave[4];
    AX_PROPAGATE_RESULT_ERROR(buffer.Read((uint8_t*) riff, 4));
    AX_PROPAGATE_ERROR(buffer.SkipRead(4));
    AX_PROPAGATE_RESULT_ERROR(buffer.Read((uint8_t*) wave, 4));
    if (std::strncmp(riff, "RIFF", 4) != 0 ||
        std::strncmp(wave, "WAVE", 4) != 0) {
        AX_PROPAGATE_ERROR(buffer.SkipRead(-12));
        return utils::ExError{"Invalid WAV file format"};
    }
    AX_PROPAGATE_ERROR(buffer.SkipRead(-12));

    WAVAudio wav;
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(wav.header.riff, 4));
    AX_PROPAGATE_ERROR(buffer.SkipRead(4));
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(wav.header.wave, 4));
    AX_PROPAGATE_RESULT_ERROR(buffer.Read(wav.header.fmt, 4));
    AX_PROPAGATE_ERROR(buffer.SkipRead(4));
    AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(buffer, &wav.header.audioFormat));
    AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(buffer, &wav.header.numChannels));
    AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint32_t>(buffer, &wav.header.sampleRate));
    AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint32_t>(buffer, &wav.header.byteRate));
    AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(buffer, &wav.header.blockAlign));
    AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(buffer, &wav.header.bitsPerSample));

    char dataHeader[4];
    uint32_t dataSize;
    do {
        AX_PROPAGATE_RESULT_ERROR(buffer.Read(dataHeader, 4));
        AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint32_t>(buffer, &dataSize));
        
        if (std::string(dataHeader, 4) != "data") {
            buffer.SkipRead(dataSize);
        }
    } while (std::string(dataHeader,4) != "data");

    wav.format = (wav.header.numChannels == 1 ?
                 (wav.header.bitsPerSample == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) :
                 (wav.header.bitsPerSample == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16));
    wav.samples.resize(dataSize);
    AX_PROPAGATE_RESULT_ERROR(buffer.Read((uint8_t*) wav.samples.data(), dataSize));

    return wav;
}

utils::ExResult<WAVAudio> WAV_LoadFileBytes(utils::Span<uint8_t> bufferView) {
    auto stream = std::make_shared<data::BufferDataStream>(bufferView);
    AX_PROPAGATE_ERROR(stream->Open());
    return WAV_LoadFileBytes(*stream);
}

utils::ExResult<WAVAudio> WAV_LoadFile(const std::filesystem::path& path) {
    auto stream = std::make_shared<data::FileDataStream>(path, true, false);
    AX_PROPAGATE_ERROR(stream->Open());
    return WAV_LoadFileBytes(*stream);
}

}