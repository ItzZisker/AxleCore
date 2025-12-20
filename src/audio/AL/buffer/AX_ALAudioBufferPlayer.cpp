#include "axle/audio/AL/buffer/AX_ALAudioBufferPlayer.hpp"

#include "axle/audio/AL/AX_ALIAudioPlayer.hpp"

#include "axle/audio/AL/buffer/AX_ALAudioBuffer.hpp"
#include "axle/audio/AL/buffer/AX_ALAudioBufferSource.hpp"

#include <cstddef>
#include <memory>

namespace axle::audio
{

ALAudioBufferPlayer::ALAudioBufferPlayer(std::size_t max_sources) : ALIAudioPlayer(max_sources) {}

ALAudioBufferSource& ALAudioBufferPlayer::Play(ALAudioBuffer& buffer) {
    auto sourceOpt = AcquireFreeSource();
    if (sourceOpt.has_value()) {
        auto source = sourceOpt.value();
        source->AttachBuffer(buffer);
        source->Play();
    }
    return *sourceOpt.value();
}

void ALAudioBufferPlayer::ApplyToSources(std::function<void(ALAudioBufferSource&)> iteration_func) {
    for (std::unique_ptr<ALAudioBufferSource>& uPtr : m_sources) {
        iteration_func(*uPtr.get());
    }
}

}