#include "axle/core/alloc/AX_LinearAllocator.hpp"

namespace axle::core
{

LinearAllocator::LinearAllocator(size_t size) {
    m_memory = (uint8_t*)std::malloc(size);
    m_size = size;
    m_offset = 0;
}

LinearAllocator::~LinearAllocator() {
    std::free(m_memory);
}

void* LinearAllocator::Allocate(size_t size, size_t alignment) {
    size_t current = (size_t)(m_memory + m_offset);
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);

    size_t newOffset = aligned - (size_t)m_memory + size;
    if (newOffset > m_size) return nullptr;

    m_offset = newOffset;
    return (void*)aligned;
}

void LinearAllocator::Reset() {
    m_offset = 0;
}

}