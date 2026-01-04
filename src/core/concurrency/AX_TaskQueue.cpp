#include "axle/core/concurrency/AX_TaskQueue.hpp"

#include <deque>
#include <mutex>

namespace axle::core {

std::deque<VoidJob> TaskQueue::CopyJobs() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Tasks;
}

void TaskQueue::Enqueue(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Tasks.push_back(std::move(task));
}

void TaskQueue::ExecuteAll() {
    std::deque<std::function<void()>> local;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::swap(local, m_Tasks);
    }
    while (!local.empty()) {
        auto& f = local.front();
        if (f) f();
        local.pop_front();
    }
}

}