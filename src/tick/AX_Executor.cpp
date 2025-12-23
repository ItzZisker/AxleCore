#include "axle/tick/AX_Executor.hpp"

#include <mutex>
#include <thread>

namespace axle::tick
{

Executor::Executor(bool instaSignalAfterQ) : m_InstaSignalExecution(instaSignalAfterQ) {}
Executor::~Executor() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Running = false;
    m_Wake = true;
    m_CV.notify_all();
}

void Executor::Init() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (IsRunning()) return;
    std::thread thr([this]() {
        std::unique_lock<std::mutex> lock(m_Mutex);

        while (true) {
            m_CV.wait(lock, [&] { return m_Wake || !m_Running; });
            if (!m_Running) break;
            m_Wake = false;
            if (lock.try_lock()) {    
                while (!m_Executions.empty()) {
                    auto func = m_Executions.front();
                    m_Executions.pop_front();
                    func();
                }
                lock.unlock();
            }
        }
    });
    m_ThreadID = thr.get_id();
}

void Executor::QueueExec(std::function<void()> func) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Executions.push_back(func);
    if (this->m_InstaSignalExecution) {
        m_Wake = true;
        m_CV.notify_all();
    }
}

void Executor::Signal() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Wake = true;
    m_CV.notify_all();
}

}