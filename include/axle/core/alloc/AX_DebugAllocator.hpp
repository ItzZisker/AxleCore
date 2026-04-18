#pragma once

#include <unordered_map>
#include <iostream>

namespace axle::core
{

struct AllocationInfo {
    size_t size;
    const char* file;
    int line;
};

class DebugAllocator {
private:
    std::unordered_map<void*, AllocationInfo> m_allocations{};
public:
    void* Allocate(size_t size, const char* file, int line);
    void Free(void* ptr);
    void ReportLeaks();
};

}