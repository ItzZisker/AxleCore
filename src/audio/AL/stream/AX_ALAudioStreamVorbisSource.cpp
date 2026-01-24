#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisSource.hpp"
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbis.hpp"

#include <AL/al.h>
#include <AL/alext.h>

namespace axle::audio
{

void ALAudioStreamVorbisSource::PurgeBuffers() {
    if (!m_Buffers.empty()) {
        alDeleteBuffers(ALsizei(m_Buffers.size()), m_Buffers.data());
        m_Buffers.clear();
    }
}

bool ALAudioStreamVorbisSource::IsAttached() {
    return (this->m_Stream != nullptr);
}

void ALAudioStreamVorbisSource::SetDetachAfterEndOfStream(bool detachAfterEnd) {
    this->m_DetachAfterEndOfStream = detachAfterEnd;
}

void ALAudioStreamVorbisSource::SetInitBufferCount(int bufferCount) {
    this->m_InitBufferCount = bufferCount;
}

ALAudioStreamVorbisSource::~ALAudioStreamVorbisSource() {
    ALAudioStreamVorbisSource::PurgeBuffers();
}

void ALAudioStreamVorbisSource::Attach(ALAudioStreamVorbis* stream) {
    if (IsAttached()) return;
    this->m_Stream = stream;
    m_Stream->Tick(1.0f);

    m_Buffers.resize(size_t(m_InitBufferCount));
    alGenBuffers(ALsizei(m_InitBufferCount), m_Buffers.data());

    for (ALuint buf : m_Buffers) {
        int16_t pcm[4096];
        size_t samples = m_Stream->PopSamples(pcm, 4096);

        alBufferData(
            buf,
            m_Stream->GetChannels() > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
            pcm,
            samples * sizeof(int16_t),
            m_Stream->GetSampleRate()
        );
        alSourceQueueBuffers(m_sourceID, 1, &buf);
    }
}

ALAudioStreamVorbis* ALAudioStreamVorbisSource::Detach() {
    if (!IsAttached()) return nullptr;
    auto temp = m_Stream;
    this->m_Stream = nullptr;
    ALAudioStreamVorbisSource::PurgeBuffers();
    return temp;
}

void ALAudioStreamVorbisSource::Tick(float dT) {
    if (!IsAttached() || !IsPlaying()) return;
    m_Stream->Tick(dT);

    ALint processed = 0;
    alGetSourcei(m_sourceID, AL_BUFFERS_PROCESSED, &processed);

    bool reachedEnd = false;
    while (processed--) {
        ALuint buf;
        alSourceUnqueueBuffers(m_sourceID, 1, &buf);

        int16_t pcm[4096];
        size_t samples = m_Stream->PopSamples(pcm, 4096);

        if (samples > 0) {
            alBufferData(
                buf,
                m_Stream->GetChannels() > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
                pcm,
                samples * sizeof(int16_t),
                m_Stream->GetSampleRate()
            );

            alSourceQueueBuffers(m_sourceID, 1, &buf);
        } else {
            reachedEnd = true;
        }
    }
    if (reachedEnd) {
        if (IsLooping()) {
            m_Stream->Reset();
            Attach(Detach());
        } else {
            if (m_DetachAfterEndOfStream) Detach();
            Stop();
        }
    }
}

}