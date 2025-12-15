#include "axle/audio/AL/AX_ALAudioContext.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <iostream>

ALCdevice* gDevice = nullptr;
ALCcontext* gContext = nullptr;

namespace axle::audio {

bool AL_CreateContext(const ALCchar *devicename) {
    if (gDevice) {
        std::cerr << "OpenAL device held onto a device context already!\n";
        return false;
    }

    gDevice = alcOpenDevice(devicename);
    if (!gDevice) {
        std::cerr << "Failed to open OpenAL device: " << (devicename == nullptr ? "Default" : std::string(devicename)) << std::endl;
        return false;
    }

    ALCint attrs[] = { ALC_HRTF_SOFT, ALC_TRUE, 0 };
    gContext = alcCreateContext(gDevice, attrs);
    if (!gContext) {
        alcCloseDevice(gDevice);
        gDevice = nullptr;
        return false;
    }

    alcMakeContextCurrent(gContext);

    ALfloat ori[] = { 0.0f, 0.0f, -1.0f,
                      0.0f, 1.0f,  0.0f };
    alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
    alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f);
    alListenerfv(AL_ORIENTATION, ori);

    return true;
}

void AL_ShutdownContext() {
    if (gContext) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(gContext);
        gContext = nullptr;
    }
    if (gDevice) {
        alcCloseDevice(gDevice);
        gDevice = nullptr;
    }
}

void *getALCdevice() {
    return gDevice;
}

void *getALCcontext() {
    return gContext;
}

}