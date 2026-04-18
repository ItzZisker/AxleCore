#pragma once

#include "axle/utils/AX_Expected.hpp"

#include <AL/alc.h>

#include <string>
#include <vector>

namespace axle::audio::alc
{
    std::vector<std::string> ListDeviceNames();

    utils::ExError CreateContext(const ALCchar *devicename);
    utils::ExError ShutdownContext();

    utils::ExResult<bool> IsDeviceConnected();
    utils::ExError TryReopenDevice();
    utils::ExError RecoverContext();

    void *GetDevice();
    void *GetContextHandle();
}