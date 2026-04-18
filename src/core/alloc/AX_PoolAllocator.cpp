#include "axle/core/alloc/AX_PoolAllocator.hpp"
    
namespace axle::core
{

PoolAllocator::PoolAllocator(size_t objectSize, size_t capacity)
{
    m_objectSize = objectSize;
    m_memory = std::malloc(objectSize * capacity);

    for (size_t i = 0; i < capacity; i++) {
        void* ptr = (char*)m_memory + i * objectSize;
        m_freeList.push_back(ptr);
    }
}

PoolAllocator::~PoolAllocator() {
    std::free(m_memory);
}

void* PoolAllocator::Allocate() {
    if (m_freeList.empty()) return nullptr;

    void* ptr = m_freeList.back();
    m_freeList.pop_back();
    return ptr;
}

void PoolAllocator::Free(void* ptr) {
    m_freeList.push_back(ptr);
}

}