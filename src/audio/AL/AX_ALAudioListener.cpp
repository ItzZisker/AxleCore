#include "axle/audio/AL/AX_ALAudioListener.hpp"

#include <cstring>

namespace axle::audio::ALAudioListener
{
    bool HasActiveContext() {
        return alcGetCurrentContext() != nullptr;
    }

    void SetPosition(float x, float y, float z) {
        alListener3f(AL_POSITION, x, y, z);
    }

    void SetVelocity(float x, float y, float z) {
        alListener3f(AL_VELOCITY, x, y, z);
    }

    void SetOrientation(
        float fx, float fy, float fz,
        float ux, float uy, float uz
    ) {
        const ALfloat orientation[6] = {
            fx, fy, fz,
            ux, uy, uz
        };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void SetGain(float gain) {
        alListenerf(AL_GAIN, gain);
    }

    void SetPosition(const float v[3]) {
        alListenerfv(AL_POSITION, v);
    }

    void SetVelocity(const float v[3]) {
        alListenerfv(AL_VELOCITY, v);
    }

    void SetOrientation(const float forward[3], const float up[3]) {
        const ALfloat orientation[6] = {
            forward[0], forward[1], forward[2],
            up[0],      up[1],      up[2]
        };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void GetPosition(float out[3]) {
        alGetListenerfv(AL_POSITION, out);
    }

    void GetVelocity(float out[3]) {
        alGetListenerfv(AL_VELOCITY, out);
    }

    void GetOrientation(float outForward[3], float outUp[3]) {
        ALfloat orientation[6];
        alGetListenerfv(AL_ORIENTATION, orientation);

        std::memcpy(outForward, orientation,       3 * sizeof(float));
        std::memcpy(outUp,      orientation + 3,   3 * sizeof(float));
    }

    float GetGain() {
        ALfloat gain;
        alGetListenerf(AL_GAIN, &gain);
        return gain;
    }
}
