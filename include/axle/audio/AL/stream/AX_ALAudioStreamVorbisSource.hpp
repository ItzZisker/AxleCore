#pragma once

#include "axle/audio/AL/AX_ALAudioSource.hpp"
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbis.hpp"

#include "AL/al.h"

#include <vector>

namespace axle::audio
{

class ALAudioStreamVorbisSource : public ALAudioSource, public tick::ITickAdapter {
private:
    ALAudioStreamVorbis* m_Stream {nullptr};
    std::vector<ALuint> m_Buffers {};

    bool m_DetachAfterEndOfStream {false};
    int m_InitBufferCount {4};

    void PurgeBuffers();
public:
    ~ALAudioStreamVorbisSource();

    bool IsAttached();

    void SetDetachAfterEndOfStream(bool detachAfterEndOfStream);
    void SetInitBufferCount(int bufferCount);

    void Attach(ALAudioStreamVorbis* stream);
    ALAudioStreamVorbis* Detach();

    void Tick(float dT) override;
};

}
