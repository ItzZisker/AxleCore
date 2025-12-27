#pragma once

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
    std::thread m_Thread {};
    std::condition_variable m_CV {};
    bool m_Running {};

    std::deque<std::function<void()>> m_Executions {};
    std::mutex m_Mutex {};
public:
    ~Executor();

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    Executor(Executor&& other) = delete;
    Executor& operator=(Executor&& other) = delete;

    void Init();
    void QueueExec(std::function<void()> func);
    void QueueFlush();
    void Signal();

    bool IsCurrentThread() const { return std::this_thread::get_id() == m_Thread.get_id(); }
    bool IsRunning() const { return m_Thread.get_id() != std::thread::id(); }
};

}