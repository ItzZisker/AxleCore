#pragma once

#include "axle/core/app/AX_IApplication.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/concurrency/AX_TaskQueue.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace axle::core {

template <typename ContextType>
class ThreadContext {
public:
    ThreadContext() {        
        std::lock_guard<std::mutex> lock(m_CycleMutex);
        m_CycleTasks = std::make_unique<TaskQueue>();
    }

    virtual ~ThreadContext() {
        Stop();
    }

    void Start(int64_t msSleep, std::function<std::unique_ptr<ContextType>()> initFunc) {
        if (m_Running.load()) return;
        m_Running.store(true);
        m_msSleep = msSleep;
        m_Thread = std::thread([this, initFunc = std::move(initFunc)]() {
            {
                std::lock_guard lock(m_CtxMutex);
                m_Ctx = initFunc();
            }
            {
                std::lock_guard lock(m_StateMutex);
                m_Started = true;
            }
            m_StateCV.notify_all();

            DoCycle();

            {
                std::lock_guard lock(m_StateMutex);
                m_Stopped = true;
            }
            m_StateCV.notify_all();
        });
    }

    void Stop(bool join = true) {
        {
            std::lock_guard lock(m_StateMutex);
            if (!m_Running.load()) return;
            m_Running.store(false);
        }
        if (join && m_Thread.joinable())
            m_Thread.join();
    }

    ContextType* GetContext() {
        std::lock_guard<std::mutex> lock(m_CtxMutex);
        return m_Ctx.get();
    }

    void EnqueueTask(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(m_CycleMutex);
        m_CycleTasks->Enqueue(std::move(func));
    }

    void PushWork(const std::string& key, std::function<void()> func) {
        std::lock_guard<std::mutex> lock(m_WorkMutex);
        m_Works[key] = std::move(func);
    }

    void PopWork(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_WorkMutex);
        m_Works.erase(key);
    }

    void AwaitStart() {
        std::unique_lock lock(m_StateMutex);
        m_StateCV.wait(lock, [&] { return m_Started; });
    }
protected:
    bool m_Stopped{false};
    bool m_Started{false};

    std::atomic_bool m_Running{false};

    std::thread m_Thread;

    std::unordered_map<std::string, std::function<void()>> m_Works;

    std::unique_ptr<TaskQueue> m_CycleTasks{nullptr};
    std::unique_ptr<ContextType> m_Ctx{nullptr};

    std::condition_variable m_StateCV{};

    std::mutex m_StateMutex{};

    std::mutex m_WorkMutex{};
    std::mutex m_CycleMutex{};
    std::mutex m_CtxMutex{};

    int64_t m_msSleep{1};

    virtual void SubCycle() = 0;

    void DoCycle() {
        ContextType* ctx = m_Ctx.get();
        auto& cycleTasks = *m_CycleTasks;
        while (m_Running.load()) {
            static thread_local std::deque<VoidJob> localTasks;
            static thread_local std::unordered_map<std::string, std::function<void()>> localWorks;

            {
                std::lock_guard<std::mutex> lock(m_CycleMutex);
                localTasks = m_CycleTasks->CopyJobs();
            }
            {
                std::lock_guard<std::mutex> lock(m_WorkMutex);
                localWorks = m_Works;
            }

            for (auto& job : localTasks) job();
            for (auto& [_, work] : localWorks) work();
            SubCycle();
            localTasks.clear();
            localWorks.clear();

            if (m_msSleep > 0) std::this_thread::sleep_for(std::chrono::milliseconds(m_msSleep));
        }
    }
};

struct AXGEmpty {};
class GenericThreadContext : public ThreadContext<AXGEmpty> {
protected:
    std::function<void(GenericThreadContext& gctx)> m_SubCycle;

    void SubCycle() override;
public:
    void StartCycle(
        int64_t mssleep,
        std::function<std::unique_ptr<AXGEmpty>()> initFunc,
        std::function<void(GenericThreadContext& gctx)> subCycle
    );
};
// TODO: Create an "ALAudioContext" Class which holds streams, music, sources by respect to audio thread ownership

class ApplicationThreadContext : public ThreadContext<IApplication> {
protected:
    void SubCycle() override;
public:
    void StartApp(std::function<std::unique_ptr<IApplication>()> initFunc);
};

class RenderThreadContext : public ThreadContext<IRenderContext> {
protected:
    void SubCycle() override;
public:
    void StartGraphics(std::function<std::unique_ptr<IRenderContext>()> initFunc);
};

}