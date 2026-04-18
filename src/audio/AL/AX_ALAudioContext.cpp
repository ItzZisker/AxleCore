#include "axle/audio/AL/AX_ALAudioContext.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <iostream>

ALCdevice* gDevice = nullptr;
ALCcontext* gContext = nullptr;

namespace axle::audio::alc
{

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

utils::ExError CreateContext(const ALCchar *devicename) {
    if (gDevice) {
        return {"OpenAL device held onto a device context already!"};
    }

    gDevice = alcOpenDevice(devicename);
    if (!gDevice) {
        return {"Failed to open OpenAL device: " + (devicename == nullptr ? "Default" : std::string(devicename))};
    }

    ALCint attrs[] = { ALC_HRTF_SOFT, ALC_TRUE, 0 };
    gContext = alcCreateContext(gDevice, attrs);
    if (!gContext) {
        alcCloseDevice(gDevice);
        gDevice = nullptr;
        return {"Failed to create OpenAL Context: alcCreateContext() returned NULL"};
    }

    alcMakeContextCurrent(gContext);

    ALfloat ori[] = { 0.0f, 0.0f, -1.0f,
                      0.0f, 1.0f,  0.0f };
    alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
    alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f);
    alListenerfv(AL_ORIENTATION, ori);

    return utils::ExError::NoError();
}

utils::ExError ShutdownContext() {
    if (gContext) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(gContext);
        gContext = nullptr;
    }
    if (gDevice) {
        alcCloseDevice(gDevice);
        gDevice = nullptr;
    }
    return utils::ExError::NoError();
}

utils::ExResult<bool> IsDeviceConnected() {
    if (!gDevice) utils::ExError("Device/Context not available yet. consider alc::CreateContext() first!");

    if (alcIsExtensionPresent(gDevice, "ALC_EXT_disconnect")) {
        ALCint connected = ALC_TRUE;
        alcGetIntegerv(gDevice, ALC_CONNECTED, 1, &connected);
        return utils::ExResult(connected == ALC_TRUE);
    }
    return utils::ExResult(true); // If extension missing assume still connected
}

utils::ExError TryReopenDevice() {
    if (!gDevice) utils::ExError("Device/Context not available yet. consider alc::CreateContext() first!");

    if (!alcIsExtensionPresent(gDevice, "ALC_SOFT_reopen_device"))
        return {"ALC_SOFT_reopen_device extension not present"};

    static thread_local auto alcReopenDeviceSOFT =
        (ALCboolean(*)(ALCdevice*, const ALCchar*, const ALCint*))
        alcGetProcAddress(gDevice, "alcReopenDeviceSOFT");

    if (!alcReopenDeviceSOFT)
        return {"thread_local alcReopenDeviceSOFT == NULL"};

    ALCint attrs[] = { ALC_HRTF_SOFT, ALC_TRUE, 0 };
    bool success = alcReopenDeviceSOFT(gDevice, nullptr, attrs) == ALC_TRUE;
    return success ? utils::ExError::NoError() : utils::ExError::ExError("Failed to Reopen Device");
}

utils::ExError RecoverContext() {
    if (!gDevice || !gContext) return {"Device/Context not available yet. consider alc::CreateContext() first!"};

    auto res = IsDeviceConnected();
    if (res.has_value()) return utils::ExError::NoError();
    else return res.error();

    if (!TryReopenDevice().IsValid()) {
        alcMakeContextCurrent(gContext);
        return utils::ExError::NoError();
    }
    // fallback: full recreation
    ShutdownContext();
    return CreateContext(nullptr);
}

void *GetDevice() {
    return gDevice;
}

void *GetContextHandle() {
    return gContext;
}

}