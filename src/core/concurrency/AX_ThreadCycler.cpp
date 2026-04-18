#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Universal.hpp"
#include "axle/utils/AX_ResTimer.hpp"

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

void ThreadCycler::Start() {
    std::lock_guard<std::mutex> lock(m_LifeCycleMutex);
    if (m_Running.load()) return;
    m_Running.store(true);
    m_Thread = std::thread([this]() {
        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_Started = true;
        }
        m_StateCV.notify_all();

        DoCycle();

        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_Stopped = true;
        }
        m_StateCV.notify_all();
    });
}

void ThreadCycler::Stop(bool join) {
    std::lock_guard<std::mutex> lock(m_LifeCycleMutex);
    {
        std::lock_guard<std::mutex> _lock(m_StateMutex);
        if (!m_Running.load()) return;
        m_Running.store(false);
        if (m_Stopped) return;
    }
    if (join && m_Thread.joinable())
        m_Thread.join();
}

ThreadId ThreadCycler::GetThreadId() {
    return m_Thread.get_id();
}

bool ThreadCycler::ValidateThread() {
    return m_Running.load() && std::this_thread::get_id() == m_Thread.get_id();
}

void ThreadCycler::EnqueueTask(std::function<void()> func) {
    std::lock_guard<std::mutex> lock(m_CycleMutex);
    m_CycleTasks.push_back(std::move(func));
}

WorkHandle ThreadCycler::CreateWork(VoidJob job, WorkId after) {
    std::lock_guard<std::mutex> lock(m_CycleMutex);
    auto res = m_CycleWorks.Reserve(after);
    res->job = job;
    res->Sign();
    return res->External();
}

void ThreadCycler::RemoveWork(WorkHandle wh) {
    std::lock_guard<std::mutex> lock(m_CycleMutex);
    m_CycleWorks.Delete(wh);
}

void ThreadCycler::MoveWorkToEnd(WorkHandle wh) {
    std::lock_guard<std::mutex> lock(m_CycleMutex);
    m_CycleWorks.ModifyOrder([whCpy = wh](std::vector<utils::MagicId>& order) {
        auto it = std::find(order.begin(), order.end(), whCpy.index);
        if (it == order.end()) return;

        order.erase(it);
        order.push_back(whCpy.index);
    });
}

void ThreadCycler::AwaitStart() {
    std::unique_lock lock(m_StateMutex);
    m_StateCV.wait(lock, [&] { return m_Started; });
}

ChNanos ThreadCycler::GetLastCycleTimeBegin() {
    std::lock_guard<std::mutex> lock(m_CycleTimeMutex);
    return m_LastCycleTimeBegin.time_since_epoch();
}

ChNanos ThreadCycler::GetLastCycleTimeEnd() {
    std::lock_guard<std::mutex> lock(m_CycleTimeMutex);
    return m_LastCycleTimeEnd.time_since_epoch();
}

ChNanos ThreadCycler::GetLastCycleTime() {
    std::lock_guard<std::mutex> lock(m_CycleTimeMutex);
    return m_LastCycleTimeEnd - m_LastCycleTimeBegin;
}

ChNanos ThreadCycler::GetCycleTimeCap() {
    std::lock_guard<std::mutex> lock(m_CycleTimeMutex);
    return m_CycleTimeCap;
}

void ThreadCycler::SetCycleTimeCap(ChNanos nano_seconds) {
    std::lock_guard<std::mutex> lock(m_CycleTimeMutex);
    m_CycleTimeCap = nano_seconds;
}

void ThreadCycler::DoCycle() {
    std::deque<VoidJob> localTasks;

    std::vector<WorkSlot> localWorkSlots;
    std::vector<WorkId> localWorkOrder;

    using namespace std::chrono;

    auto cycleBegin = steady_clock::now(), cycleEnd = steady_clock::now();

    while (m_Running.load(std::memory_order_relaxed)) {
        {
            std::lock_guard<std::mutex> lock(m_CycleMutex);
            localTasks = m_CycleTasks;
            m_CycleTasks.clear();

            const auto& workSlots = m_CycleWorks.GetInternal();
            const auto& workOrder = m_CycleWorks.GetOrder();
            localWorkSlots = workSlots;
            localWorkOrder = workOrder;
        }

        cycleBegin = steady_clock::now();
        for (auto& job : localTasks) {
            job();
        }
        for (const WorkId& hId : localWorkOrder) {
            localWorkSlots[hId].job();
        }
        cycleEnd = steady_clock::now();

        auto cycleTimeCap{ChNanos(0)};
        {
            std::lock_guard<std::mutex> lock(m_CycleTimeMutex);
            cycleTimeCap = m_CycleTimeCap;
        }

        auto targetTime = cycleBegin + cycleTimeCap;
        utils::Uni_NanoSleep(targetTime - steady_clock::now());

        {
            std::lock_guard<std::mutex> lock(m_CycleTimeMutex);

            m_LastCycleTimeBegin = cycleBegin;
            m_LastCycleTimeEnd = steady_clock::now();
        }
    }
}

utils::ExError ThreadContextGeneric::StartCycle(
    std::function<SharedPtr<EmptyStruct>()> initFunc,
    std::function<void(EmptyStruct& gctx)> constWork
) {
    auto constWk = [this, constWork = constWork](){ if (m_Ctx) constWork(*m_Ctx.get()); };
    return StartSyncd(std::move(initFunc), std::move(constWk));
}

utils::ExError ThreadContextWnd::StartApp(CtxCreatorFunc initFunc) {
    auto constWk = [this](){ if (m_Ctx) m_Ctx->PollEvents(); };
    return StartSyncd(std::move(initFunc), std::move(constWk));
}

void ThreadContextWnd::SignalWindow() {
    m_Ctx->RequestWakeEventloop();
}

utils::ExError ThreadContextGfx::StartGfx(CtxCreatorFunc initFunc, float frameCap, bool autoPresent) {
    m_IsAutoPresent.store(autoPresent);
    m_FrameCap.store(frameCap);
    
    using namespace std::chrono;

    auto constWk = [this]() {
        if (!m_Ctx) return;

        using namespace std::chrono;

        static thread_local auto frameStart = steady_clock::now();

        if (m_IsAutoPresent.load(std::memory_order_relaxed)) {
            auto img = m_Ctx->AcquireNextImage();

            if (img.has_value()) {
                auto perr = m_Ctx->Present(img.value());
                if (!perr.IsNoError()) std::cerr << "AX Error (AutoPresent): " << perr.GetMessage() << std::endl;
            } else {
                std::cerr << "AX Error (AcquireNextImage): " << img.error().GetMessage() << std::endl;
            }
        }

        auto now = steady_clock::now();
        auto delta = now - frameStart;

        float frameCap = m_FrameCap.load();

        if (frameCap > 0.0f) {
            auto targetFrame = nanoseconds((int64_t)((1.0 / frameCap) * 1e9));
            auto targetTime = frameStart + targetFrame;

            utils::Uni_NanoSleep(targetTime - steady_clock::now());

            now = steady_clock::now();
            delta = now - frameStart;
        }
        frameStart = steady_clock::now();

        float deltaF = duration<float>(delta).count();

        m_FrameTime.store(deltaF);
        m_AccumulatedTime.fetch_add(deltaF);
    };

    return StartSyncd(std::move(initFunc), std::move(constWk));
}

}