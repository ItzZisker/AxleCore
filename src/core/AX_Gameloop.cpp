#include "axle/core/AX_GameLoop.hpp"
#include "axle/core/concurrency/AX_TaskQueue.hpp"
#include "axle/core/window/AX_IWindow.hpp"
#include "axle/utils/AX_Types.hpp"

namespace axle::core
{

ThreadCycler::ThreadCycler(WorkOrder order) : m_Works(order) {
    std::lock_guard<std::mutex> lock(m_CycleMutex);
    m_CycleTasks = std::make_unique<TaskQueue>();
}

ThreadCycler::~ThreadCycler() {
    Stop();
}

void ThreadCycler::Start(int64_t msSleep, std::function<void()> initFunc) {
    std::lock_guard lock(m_LifeCycleMutex);
    if (m_Running.load()) return;
    m_Running.store(true);
    m_msSleep = msSleep;
    m_Thread = std::thread([this, initFunc = std::move(initFunc)]() {
        {
            std::lock_guard lock(m_InitMutex);
            initFunc();
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

void ThreadCycler::Stop(bool join) {
    std::lock_guard lock(m_LifeCycleMutex);
    {
        std::lock_guard lock(m_StateMutex);
        if (!m_Running.load()) return;
        m_Running.store(false);
        if (m_Stopped) return;
    }
    if (join && m_Thread.joinable())
        m_Thread.join();
}

void ThreadCycler::ValidateThread() {
    if (!m_Running.load()) {
        throw std::runtime_error("AX Exception: ContextThread not running!");
    }
    if (std::this_thread::get_id() != m_Thread.get_id()) {
        throw std::runtime_error("AX Exception: ContextThread Validation Error");
    }
}

void ThreadCycler::EnqueueTask(std::function<void()> func) {
    std::lock_guard<std::mutex> lock(m_CycleMutex);
    m_CycleTasks->Enqueue(std::move(func));
}

void ThreadCycler::PushWork(const std::string& key, std::function<void()> func) {
    std::lock_guard<std::mutex> lock(m_WorkMutex);
    m_Works[key] = std::move(func);
}

void ThreadCycler::PopWork(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_WorkMutex);
    m_Works.erase(key);
}

void ThreadCycler::SetWorkOrder(WorkOrder order) {
    std::map<std::string, VoidJob, WorkOrder> rebuild(m_Works.begin(), m_Works.end(), order);
    m_Works.swap(rebuild);
}

void ThreadCycler::AwaitStart() {
    std::unique_lock lock(m_StateMutex);
    m_StateCV.wait(lock, [&] { return m_Started; });
}

void ThreadCycler::DoCycle() {
    auto& cycleTasks = *m_CycleTasks;

    std::deque<VoidJob> localTasks;
    std::map<std::string, VoidJob, WorkOrder> localWorks;

    while (m_Running.load()) {
        {
            std::lock_guard<std::mutex> lock(m_CycleMutex);
            localTasks = m_CycleTasks->MoveJobs();
        }
        {
            std::lock_guard<std::mutex> lock(m_WorkMutex);
            localWorks = m_Works;
        }

        for (auto& job : localTasks) job();
        for (auto& [_, work] : localWorks) work();
        localTasks.clear();
        localWorks.clear();

        if (m_msSleep > 0) std::this_thread::sleep_for(std::chrono::milliseconds(m_msSleep));
    }
}

void ThreadContextGeneric::StartCycle(
    int64_t mssleep,
    std::function<std::shared_ptr<EmptyStruct>()> initFunc,
    std::function<void(ThreadContextGeneric& gctx)> constWork
) {
    Start(mssleep, std::move(initFunc));
    PushWork(AX_WKKEY_CONSTANT, [this, wk = std::move(constWork)]{ wk(*this); });
}

void ThreadContextWnd::StartApp(std::function<std::shared_ptr<IWindow>()> initFunc) {
    Start(1, std::move(initFunc));
    PushWork(AX_WKKEY_CONSTANT, [this]{ if (m_Ctx) m_Ctx->PollEvents(); });
}

// swapbuffers already introduce GPU-synch block which will push the GPU and CPU into battle and gives maximum framerate
void ThreadContextGfx::StartGfx(std::function<std::shared_ptr<IRenderContext>()> initFunc) {
    Start(0, std::move(initFunc)); // so, based on explanation above; mssleep becomes 0
    PushWork(AX_WKKEY_CONSTANT, [this]{ if (m_Ctx) m_Ctx->SwapBuffers(); });
}

}