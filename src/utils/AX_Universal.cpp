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

unsigned int Decode85Byte(char c) {
    return c >= '\\' ? c - 36 : c - 35;
}

void Decode85(const unsigned char* src, unsigned char* dst, std::size_t size) {
    for (size_t i{0}; i < size; i++) {
        unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);
        src += 5;
        dst += 4;
    }
} 

}