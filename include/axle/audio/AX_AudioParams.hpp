#pragma once

#include "axle/utils/AX_Span.hpp"
#include "axle/utils/AX_MagicPool.hpp"

#include <cstdint>

/*
    NOTE:
        - We do not internally enforce source is relative based on channels > 1
          so be sure to always use the right source for right use.

        - However if UI/music buffer/stream is stereo ensure the attached source has
          set source.relative = true

        Mono buffers   -> spatial sources
        Stereo buffers -> relative sources (UI/music)
*/

namespace axle::audio
{

template<typename Tag>
struct ExternalHandle : public utils::MagicHandleTagged<Tag> {};

enum class PCMFormat {
    Mono8,
    Mono16,
    MonoFloat32,

    Stereo8,
    Stereo16,
    StereoFloat32
};

inline bool PCM_IsStereo(PCMFormat format) {
    return format >= PCMFormat::Stereo8;
}

inline uint32_t PCM_ChannelCount(PCMFormat format) {
    return PCM_IsStereo(format) ? 2 : 1;
}

struct BufferDesc {
    PCMFormat format{PCMFormat::Mono16};
    int32_t frequency{44100};
    int32_t size{0};
};

struct BufferTag {};
using BufferHandle = ExternalHandle<BufferTag>;

enum class DistanceModel {
    None,
    Inverse,
    InverseClamped,
    Linear,
    LinearClamped,
    Exponent,
    ExponentClamped
};

struct AudioGlobalsDesc {
    DistanceModel distanceModel{DistanceModel::InverseClamped};
    float dopplerFactor{1.0f};
    float speedOfSound{343.3f};
};

enum class EffectType {
    Reverb,
    Chorus,
    Distortion,
    Echo,
    Flanger,
    FrequencyShifter,
    VocalMorpher,
    PitchShifter,
    RingModulator,
    Autowah,
    Compressor,
    Equalizer
};

struct EffectDesc {
    EffectType type{};
    float params[16]{};
    uint32_t paramCount{0};
};

struct EffectTag {};
using EffectHandle = ExternalHandle<EffectTag>;

struct EffectSlotDesc {
    EffectHandle effect{};
    bool effectSet{false};

    float gain{1.0f};
    bool autoSend{true};
};

struct EffectSlotTag {};
using EffectSlotHandle = ExternalHandle<EffectSlotTag>;

enum class FilterType {
    LowPass,
    HighPass,
    BandPass
};

struct FilterDesc {
    FilterType type{};

    float gain{1.0f};
    float gainHF{1.0f};
    float gainLF{1.0f};
};

struct FilterTag {};
using FilterHandle = ExternalHandle<FilterTag>;

struct AuxSendDesc {
    EffectSlotHandle slot{};
    bool slotSet{false};

    FilterHandle filter{};
    bool filtering{false};
};

struct SourceAuxRouting {
    utils::CowSpan<AuxSendDesc> sends{};
    uint32_t sendCount{0};
};

struct BusTag {};
using BusHandle = ExternalHandle<BusTag>;

struct BusDesc {
    BusHandle parent{};
    bool parentSet{false};

    float gain{1.0f};
    bool mute{false};
};

struct SourceOptions {
    bool loop{false};
    float pitch{1.0f};
    float gain{1.0f};

    float position[3]{0.0f, 0.0f, 0.0f};
    float velocity[3]{0.0f, 0.0f, 0.0f};
    float direction[3]{0.0f, 0.0f, -1.0f};

    bool relative{false};

    float referenceDistance{1.0f};
    float maxDistance{1000.0f};
    float rolloffFactor{1.0f};

    float coneInnerAngle{360.0f};
    float coneOuterAngle{360.0f};
    float coneOuterGain{0.0f};

    float dopplerFactor{1.0f};

    float airAbsorptionFactor{0.0f};

    FilterHandle directFilter{};
    bool directFiltering{false};

    SourceAuxRouting aux;

    BusHandle bus{};
    bool hasBus{false};

    uint32_t priority{0};
};

struct SourceDesc {
    SourceOptions options{};
};

struct SourceTag {};
using SourceHandle = ExternalHandle<SourceTag>;

struct ListenerDesc {
    float position[3]{0.0f, 0.0f, 0.0f};
    float velocity[3]{0.0f, 0.0f, 0.0f};

    float orientation[6]{
        0.0f, 0.0f, -1.0f,
        0.0f, 1.0f,  0.0f
    };

    float gain{1.0f};
};

struct ListenerTag {};
using ListenerHandle = ExternalHandle<ListenerTag>;

struct StreamDesc {
    int32_t channels{2};
    int32_t frequency{44100};
    int32_t size{0};

    float bufferSeconds{2.0f};
    float periodSeconds{0.25f};

    bool loop{false};
};

struct StreamTag {};
using StreamHandle = ExternalHandle<StreamTag>;

}
