#pragma once

#include <vector>
#include <cstdint>

struct ASTCImage {
    int width;
    int height;
    int depth;
    int blockX, blockY, blockZ;
    std::vector<uint8_t> data;
};