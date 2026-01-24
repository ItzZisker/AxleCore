#pragma once

#include <AL/alc.h>

namespace axle::audio {
    bool AL_CreateContext(const ALCchar *devicename);
    void AL_ShutdownContext();

    void *GetALCdevice();
    void *GetALCcontext();
}