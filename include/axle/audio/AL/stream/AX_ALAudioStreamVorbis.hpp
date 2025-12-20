#pragma once

#include "axle/audio/AL/stream/AX_ALIAudioStream.hpp"

#include "axle/audio/data/AX_AudioOGG.hpp"
#include "axle/data/AX_IDataStream.hpp"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <memory>
#include <vector>

namespace axle::audio
{

class ALAudioStreamVorbis : public ALIAudioStream {
private:
    OGGAudio& m_Data;

    stb_vorbis* m_Vorbis {nullptr};
    stb_vorbis_info m_Info {};
public:
    explicit ALAudioStreamVorbis(OGGAudio& data, AudioStreamDesc desc = {});
    ~ALAudioStreamVorbis();

    void Tick(float dT) override;

    size_t PopSamples(int16_t* out, size_t samples) override;

    void SeekSeconds(float seconds) override;
    void SeekSamples(uint64_t sampleIndex) override;

    void Reset() override;

};

}