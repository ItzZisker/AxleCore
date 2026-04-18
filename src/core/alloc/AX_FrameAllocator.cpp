#include "axle/core/alloc/AX_FrameAllocator.hpp"

namespace axle::core
{

FrameAllocator::FrameAllocator(size_t size)
    : m_arena(size) {}

void* FrameAllocator::Allocate(size_t size, size_t align) {
    return m_arena.Allocate(size, align);
}

void FrameAllocator::BeginFrame() {
    m_arena.Reset();
}

}