#pragma once
#ifdef __AX_AUDIO_ALSOFT__

#include "axle/audio/AX_AudioParams.hpp"
#include "axle/audio/cmd/AX_IAudioBackend.hpp"

#include "axle/utils/AX_MagicPool.hpp"

#include "AL/al.h"
#include "AL/alc.h"

#include <vector>
#include <cstdint>

#ifndef AL_SOFT_source_resampler
#define AL_SOFT_source_resampler 1

#define AL_SOURCE_RESAMPLER_SOFT    0x1214
#define AL_NUM_RESAMPLERS_SOFT      0x1215
#define AL_RESAMPLER_NAME_SOFT      0x1216

#define AL_RESAMPLER_POINT_SOFT     0x0000
#define AL_RESAMPLER_LINEAR_SOFT    0x0001
#define AL_RESAMPLER_CUBIC_SOFT     0x0002
#endif

#ifndef AL_FRONT_LEFT
#define AL_FRONT_LEFT               0x0001
#define AL_FRONT_RIGHT              0x0002
#define AL_FRONT_CENTER             0x0004
#define AL_LOW_FREQUENCY            0x0008
#define AL_BACK_LEFT                0x0010
#define AL_BACK_RIGHT               0x0020
#define AL_FRONT_LEFT_OF_CENTER     0x0040
#define AL_FRONT_RIGHT_OF_CENTER    0x0080
#define AL_BACK_CENTER              0x0100
#define AL_SIDE_LEFT                0x0200
#define AL_SIDE_RIGHT               0x0400
#define AL_TOP_CENTER               0x0800
#define AL_TOP_FRONT_LEFT           0x1000
#define AL_TOP_FRONT_CENTER         0x2000
#define AL_TOP_FRONT_RIGHT          0x4000
#define AL_TOP_BACK_LEFT            0x8000
#define AL_TOP_BACK_CENTER          0x10000
#define AL_TOP_BACK_RIGHT           0x20000
#endif

namespace axle::audio
{

template <typename T_Extern>
struct ALInternal : public utils::MagicInternal<T_Extern> {};

struct ALBuffer : public ALInternal<BufferHandle> {
    ALuint id{0};
    BufferDesc desc{};
};

struct ALSource : public ALInternal<SourceHandle> {
    ALuint id{0};

    BufferHandle buffer{};
    StreamHandle stream{};

    SourceOptions options{};
};

struct ALStream : public ALInternal<StreamHandle> {
    std::vector<ALuint> buffers{};
    StreamDesc desc{};

    int channels{0};
    int frequency{0};

    bool playing{false};
};

struct ALEffect : public ALInternal<EffectHandle> {
    ALuint id{0};
    EffectDesc desc{};
};

struct ALEffectSlot : public ALInternal<EffectSlotHandle> {
    ALuint id{0};
    EffectSlotDesc desc{};
};

struct ALFilter : public ALInternal<FilterHandle> {
    ALuint id{0};
    FilterDesc desc{};
};

struct ALBus : public ALInternal<BusHandle> {
    BusDesc desc{};
};

class ALAudioBackend final : public IAudioBackend {
private:
    ALCdevice* m_Device{nullptr};
    ALCcontext* m_Context{nullptr};
public:
    ALAudioBackend();
    ~ALAudioBackend() override;

    utils::ExError Initialize() override;
    utils::ExError Shutdown() override;

    utils::ExError UpdateGlobals(const AudioGlobalsDesc& desc) override;

    utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc, const void* data) override;
    utils::ExError DestroyBuffer(const BufferHandle& handle) override;

    utils::ExResult<StreamHandle> CreateStream(const StreamDesc& desc) override;
    utils::ExError DestroyStream(const StreamHandle& handle) override;

    utils::ExResult<SourceHandle> CreateSource(const SourceDesc& desc) override;
    utils::ExError UpdateSource(const SourceHandle& handle, const SourceOptions& options) override;
    utils::ExError DestroySource(const SourceHandle& handle) override;

    utils::ExError AttachBuffer(const SourceHandle& source, const BufferHandle& buffer) override;
    utils::ExError AttachStream(const SourceHandle& source, const StreamHandle& stream) override;

    utils::ExError UpdateListener(const ListenerDesc& desc) override;

    utils::ExResult<EffectHandle> CreateEffect(const EffectDesc& desc) override;
    utils::ExError DestroyEffect(const EffectHandle& handle) override;

    utils::ExResult<EffectSlotHandle> CreateEffectSlot(const EffectSlotDesc& desc) override;
    utils::ExError DestroyEffectSlot(const EffectSlotHandle& handle) override;

    utils::ExResult<FilterHandle> CreateFilter(const FilterDesc& desc) override;
    utils::ExError DestroyFilter(const FilterHandle& handle) override;

    utils::ExResult<BusHandle> CreateBus(const BusDesc& desc) override;
    utils::ExError DestroyBus(const BusHandle& handle) override;

    utils::ExError Update(float dT) override;
private:
    utils::MagicPool<ALBuffer>      m_Buffers{};
    utils::MagicPool<ALSource>      m_Sources{};
    utils::MagicPool<ALStream>      m_Streams{};
    utils::MagicPool<ALEffect>      m_Effects{};
    utils::MagicPool<ALEffectSlot>  m_EffectSlots{};
    utils::MagicPool<ALFilter>      m_Filters{};
    utils::MagicPool<ALBus>         m_Buses{};
};


}
#endif
