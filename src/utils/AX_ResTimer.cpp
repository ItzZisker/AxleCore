#include "axle/utils/AX_ResTimer.hpp"

#include <windows.h>

fwResTimer::fwResTimer(unsigned int resolution) : _resolution(resolution), _active(false) {
    if (timeBeginPeriod(_resolution) == TIMERR_NOERROR) {
        _active = true;
    }
}

fwResTimer::~fwResTimer() {
    if (_active) {
        timeEndPeriod(_resolution);
    }
}

fwResTimer::fwResTimer(fwResTimer&& other) noexcept : _resolution(other._resolution), _active(other._active) {
    other._active = false;
}