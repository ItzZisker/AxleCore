#include "axle/core/AX_GameLoop.hpp"
#include "axle/core/concurrency/AX_TaskQueue.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/window/AX_IWindow.hpp"
#include "axle/utils/AX_Types.hpp"

#include <atomic>
#include <functional>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <vector>

namespace axle::core
{

ThreadCycler::~ThreadCycler() {
    Stop();
}

void ThreadCycler::Start(int64_t sleepMS) {
    std::lock_guard lock(m_LifeCycleMutex);
    if (m_Running.load()) return;
    m_Running.store(true);
    m_SleepMS = sleepMS;
    m_Thread = std::thread([this]() {
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
    m_CycleTasks.Enqueue(std::move(func));
}

WorkHandle ThreadCycler::CreateWork(VoidJob job, WorkHandle after) {
    std::lock_guard lock(m_WorkMutex);

    WorkHandle h;
    if (!m_WorkFree.empty()) {
        h = m_WorkFree.back();
        m_WorkFree.pop_back();
        m_WorkSlots[h] = { std::move(job), true };
    } else {
        h = static_cast<WorkHandle>(m_WorkSlots.size());
        m_WorkSlots.push_back({ std::move(job), true });
    }

    if (after == UINT32_MAX) {
        m_WorkOrder.push_back(h);
    } else {
        auto it = std::find(m_WorkOrder.begin(), m_WorkOrder.end(), after);
        m_WorkOrder.insert(it == m_WorkOrder.end() ? m_WorkOrder.end() : std::next(it), h);
    }
    return h;
}

void ThreadCycler::RemoveWork(WorkHandle wh) {
    std::lock_guard lock(m_WorkMutex);

    if (wh >= m_WorkSlots.size() || !m_WorkSlots[wh].alive)
        return;

    m_WorkSlots[wh].alive = false;
    m_WorkSlots[wh].job = nullptr;
    m_WorkFree.push_back(wh);

    auto it = std::remove(m_WorkOrder.begin(), m_WorkOrder.end(), wh);
    m_WorkOrder.erase(it, m_WorkOrder.end());
}

void ThreadCycler::MoveWorkToEnd(WorkHandle wh) {
    std::lock_guard lock(m_WorkMutex);

    auto it = std::find(m_WorkOrder.begin(), m_WorkOrder.end(), wh);
    if (it == m_WorkOrder.end()) return;

    m_WorkOrder.erase(it);
    m_WorkOrder.push_back(wh);
}

void ThreadCycler::AwaitStart() {
    std::unique_lock lock(m_StateMutex);
    m_StateCV.wait(lock, [&] { return m_Started; });
}

void ThreadCycler::DoCycle() {
    std::deque<VoidJob> localTasks;

    std::vector<WorkHandle> localOrder;
    std::vector<WorkSlot> localSlots;

    while (m_Running.load(std::memory_order_relaxed)) {
        {
            std::lock_guard<std::mutex> lock(m_CycleMutex);
            localTasks = m_CycleTasks.MoveJobs();
        }
        {
            std::lock_guard lock(m_WorkMutex);
            localOrder = m_WorkOrder;
            localSlots = m_WorkSlots;
        }

        for (auto& job : localTasks) {
            job();
        }
        for (WorkHandle h : localOrder) {
            const WorkSlot& slot = localSlots[h];
            if (slot.alive && slot.job) slot.job();
        }

        if (m_SleepMS > 0) std::this_thread::sleep_for(std::chrono::milliseconds(m_SleepMS));
    }
}

bool ThreadContextGeneric::StartCycle(
    int64_t sleepMS,
    std::function<SharedPtr<EmptyStruct>()> initFunc,
    std::function<void(EmptyStruct& gctx)> constWork
) {
    return Start(sleepMS, std::move(initFunc), std::move(constWork));
}

bool ThreadContextWnd::StartApp(CtxCreatorFunc initFunc, int64_t sleepMS) {
    auto constWk = [this](IWindow&){ if (m_Ctx) m_Ctx->PollEvents(); };
    return Start(sleepMS, std::move(initFunc), std::move(constWk));
}

// swapbuffers already introduce GPU-synch block which will push the GPU and CPU into battle and gives maximum framerate
bool ThreadContextGfx::StartGfx(CtxCreatorFunc initFunc) {
    auto constWk = [this](IRenderContext&){ if (m_Ctx) m_Ctx->SwapBuffers(); };
    return Start(0, std::move(initFunc), std::move(constWk)); // so, based on explanation above; mssleep becomes 0
}

}