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

#ifdef __AX_ASSETS_ASSIMP__
glm::mat4 Assimp_ToGLM(const aiMatrix4x4& aiMat) {
    glm::mat4 mat;
    mat[0][0] = aiMat.a1; mat[1][0] = aiMat.a2; mat[2][0] = aiMat.a3; mat[3][0] = aiMat.a4;
    mat[0][1] = aiMat.b1; mat[1][1] = aiMat.b2; mat[2][1] = aiMat.b3; mat[3][1] = aiMat.b4;
    mat[0][2] = aiMat.c1; mat[1][2] = aiMat.c2; mat[2][2] = aiMat.c3; mat[3][2] = aiMat.c4;
    mat[0][3] = aiMat.d1; mat[1][3] = aiMat.d2; mat[2][3] = aiMat.d3; mat[3][3] = aiMat.d4;
    return mat;
}
#endif

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