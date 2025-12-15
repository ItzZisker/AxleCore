#pragma once

#include <AL/al.h>
#include <AL/alc.h>

namespace axle::audio::ALAudioListener
{
    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);

    void SetOrientation(
        float forwardX, float forwardY, float forwardZ,
        float upX,      float upY,      float upZ
    );
    void SetGain(float gain);

    void SetPosition(const float v[3]);
    void SetVelocity(const float v[3]);
    void SetOrientation(const float forward[3], const float up[3]);

    void GetPosition(float out[3]);
    void GetVelocity(float out[3]);
    void GetOrientation(float outForward[3], float outUp[3]);
    float GetGain();

    bool HasActiveContext();
}
