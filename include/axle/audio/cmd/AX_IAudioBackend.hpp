#pragma once

#include "axle/audio/AX_AudioParams.hpp"

#include "axle/utils/AX_Expected.hpp"

namespace axle::audio
{

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    virtual utils::ExError Initialize() = 0;
    virtual utils::ExError Shutdown() = 0;

    virtual utils::ExError UpdateGlobals(const AudioGlobalsDesc& desc) = 0;

    virtual utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc, const void* data) = 0;
    virtual utils::ExError DestroyBuffer(const BufferHandle& handle) = 0;

    virtual utils::ExResult<StreamHandle> CreateStream(const StreamDesc& desc) = 0;
    virtual utils::ExError DestroyStream(const StreamHandle& handle) = 0;

    virtual utils::ExResult<SourceHandle> CreateSource(const SourceDesc& desc) = 0;
    virtual utils::ExError UpdateSource(const SourceHandle& handle, const SourceOptions& options) = 0;
    virtual utils::ExError DestroySource(const SourceHandle& handle) = 0;

    virtual utils::ExError AttachBuffer(const SourceHandle& source, const BufferHandle& buffer) = 0;
    virtual utils::ExError AttachStream(const SourceHandle& source, const StreamHandle& stream) = 0;

    virtual utils::ExError UpdateListener(const ListenerDesc& desc) = 0;

    virtual utils::ExResult<EffectHandle> CreateEffect(const EffectDesc& desc) = 0;
    virtual utils::ExError DestroyEffect(const EffectHandle& handle) = 0;

    virtual utils::ExResult<EffectSlotHandle> CreateEffectSlot(const EffectSlotDesc& desc) = 0;
    virtual utils::ExError DestroyEffectSlot(const EffectSlotHandle& handle) = 0;

    virtual utils::ExResult<FilterHandle> CreateFilter(const FilterDesc& desc) = 0;
    virtual utils::ExError DestroyFilter(const FilterHandle& handle) = 0;

    virtual utils::ExResult<BusHandle> CreateBus(const BusDesc& desc) = 0;
    virtual utils::ExError DestroyBus(const BusHandle& handle) = 0;

    virtual utils::ExError Update(float dT) = 0;
};

}