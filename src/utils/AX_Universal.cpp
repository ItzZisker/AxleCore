#include "axle/utils/AX_Universal.hpp"

#include "axle/utils/AX_Types.hpp"

#include <chrono>
#include <iostream>

namespace axle::utils
{

void Uni_NanoSleep(ChNanos nanos) {
    using namespace std::chrono;
    if (nanos <= ChNanos(0)) return;

    auto targetTime = steady_clock::now() + nanos;

    while (true) {
        auto remaining = targetTime - steady_clock::now();

        if (remaining <= nanoseconds(0))
            break;

        if (remaining > milliseconds(15)) {
            std::this_thread::sleep_for(milliseconds(2));
        } else if (remaining > microseconds(100)) {
            std::this_thread::yield();
        } else {
            while (steady_clock::now() < targetTime)
                _mm_pause();
            break;
        }
    }
}

}