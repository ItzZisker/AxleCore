#pragma once

#include <vector>

namespace axle::core
{

class PoolAllocator {
private:
    void* m_memory;
    size_t m_objectSize;
    std::vector<void*> m_freeList;
public:
    PoolAllocator(size_t objectSize, size_t capacity);
    ~PoolAllocator();

    void* Allocate();
    void Free(void* ptr);
};

}