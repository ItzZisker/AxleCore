#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

namespace axle::tick
{

class Executor
{
private:
    std::thread::id m_ThreadID {};
    std::condition_variable m_CV {};
    std::atomic_bool m_Running {}, m_Wake {};

    std::deque<std::function<void()>> m_Executions {};
    std::mutex m_Mutex {};

    const std::atomic_bool m_InstaSignalExecution {};
public:
    Executor(bool instaSignalExecution = false);
    ~Executor();

    void Init();
    void QueueExec(std::function<void()> func);
    void Signal();

    bool IsCurrentThread() const { return std::this_thread::get_id() == m_ThreadID; }
    bool IsRunning() const { return m_ThreadID != std::thread::id(); }
};

}