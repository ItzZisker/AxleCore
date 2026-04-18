#pragma once

#include <cstdint>
#include <cstdlib>
#include <cassert>

namespace axle::core
{

class LinearAllocator {
private:
    uint8_t* m_memory;
    size_t m_size;
    size_t m_offset;
public:
    LinearAllocator(size_t size);
    ~LinearAllocator();

    void* Allocate(size_t size, size_t alignment = 8);
    void Reset();
};

}