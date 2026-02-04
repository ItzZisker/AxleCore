#include "axle/audio/AL/AX_ALAudioContext.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <iostream>

ALCdevice* gDevice = nullptr;
ALCcontext* gContext = nullptr;

namespace axle::audio::alc {

std::vector<std::string> ListDeviceNames() {
    std::vector<std::string> devices;

    if (!alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT")) {
        return devices;
    }
    const ALCchar* deviceList = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

    if (!deviceList) return devices;

    const char* ptr = deviceList;
    while (*ptr) {
        devices.emplace_back(ptr);
        ptr += devices.back().size() + 1;
    }
    return devices;
}

bool CreateContext(const ALCchar *devicename) {
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

void ShutdownContext() {
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

void *GetDevice() {
    return gDevice;
}

void *GetContextHandle() {
    return gContext;
}

}