#include "axle/tick/AX_Executor.hpp"

#include <mutex>
#include <thread>

namespace axle::tick
{

Executor::~Executor() {
    {
        std::lock_guard lock(m_Mutex);
        m_Running = false;
    }
    m_CV.notify_all();
    if (m_Thread.joinable())
        m_Thread.join();
}

void Executor::Init() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (IsRunning()) return;
    m_Running = true;
    m_Thread = std::thread([this]() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(m_Mutex);
                m_CV.wait(lock, [&] {
                    return !m_Executions.empty() || !m_Running;
                });

                if (!m_Running && m_Executions.empty())
                    break;

                task = std::move(m_Executions.front());
                m_Executions.pop_front();
            }
            task();
        }
    });
}

void Executor::QueueExec(std::function<void()> func) {
    {
        std::lock_guard lock(m_Mutex);
        m_Executions.push_back(std::move(func));
    }
    m_CV.notify_one();
}

void Executor::QueueFlush() {
    std::deque<std::function<void()>> local;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        local.swap(m_Executions);
    }

    for (auto& func : local) {
        func();
    }
}

void Executor::Signal() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CV.notify_all();
}

}