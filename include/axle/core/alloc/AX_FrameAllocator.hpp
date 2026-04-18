#pragma once

#include "axle/core/alloc/AX_LinearAllocator.hpp"

namespace axle::core
{

class FrameAllocator {
private:
    LinearAllocator m_arena;
public:
    FrameAllocator(size_t size);

    void* Allocate(size_t size, size_t align = 8);
    void BeginFrame();
};

}