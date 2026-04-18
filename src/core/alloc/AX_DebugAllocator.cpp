#include "axle/core/alloc/AX_DebugAllocator.hpp"

namespace axle::core
{
 
void* DebugAllocator::Allocate(size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    m_allocations[ptr] = {size, file, line};

    return ptr;
}

void DebugAllocator::Free(void* ptr) {
    auto it = m_allocations.find(ptr);

    if (it != m_allocations.end())
        m_allocations.erase(it);

    std::free(ptr);
}

void DebugAllocator::ReportLeaks() {
    for (auto& [ptr, info] : m_allocations) {
        std::cout << "Leak: " << info.size << " bytes at " << info.file << ":" << info.line << "\n";
    }
}

}