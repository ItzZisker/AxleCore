#pragma once

#include <AL/alc.h>

#include <string>
#include <vector>

namespace axle::audio::alc {
    std::vector<std::string> ListDeviceNames();

    bool CreateContext(const ALCchar *devicename);
    void ShutdownContext();

    void *GetDevice();
    void *GetContextHandle();
}