#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.hpp"
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbis.hpp"
#include "axle/audio/AL/stream/AX_ALAudioStreamVorbisSource.hpp"

#include <cstddef>
#include <optional>

namespace axle::audio
{

ALAudioStreamVorbisPlayer::ALAudioStreamVorbisPlayer(std::size_t max_sources) : ALIAudioPlayer(max_sources) {}

ALAudioStreamVorbisSource* ALAudioStreamVorbisPlayer::Play(ALAudioStreamVorbis* stream) {
    auto sourceOpt = AcquireFreeSource();
    if (sourceOpt.has_value()) {
        auto source = sourceOpt.value();
        source->Detach();
        source->Attach(stream);
        source->Play();
    }
    return sourceOpt.value();
}

void ALAudioStreamVorbisPlayer::ApplyToSources(std::function<void(ALAudioStreamVorbisSource&)> iteration_func) {
    for (std::unique_ptr<ALAudioStreamVorbisSource>& uPtr : m_sources) {
        iteration_func(*uPtr.get());
    }
}

void ALAudioStreamVorbisPlayer::Tick(float dT) {
    for (auto& m_source : m_sources) {   
        m_source->Tick(dT);
    }
}

}