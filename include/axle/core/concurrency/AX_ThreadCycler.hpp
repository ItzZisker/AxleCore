#pragma once

#include "axle/core/window/AX_IWindow.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/concurrency/AX_TaskQueue.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>

namespace axle::core {

struct WorkSlot {
    VoidJob job{};
    bool alive{false};
};

class ThreadCycler {
protected:
    std::atomic_bool m_Running{false};
    bool m_Stopped{false};
    bool m_Started{false};

    TaskQueue m_CycleTasks{};

    std::thread m_Thread{};

    std::vector<WorkSlot> m_WorkSlots;
    std::vector<WorkHandle> m_WorkOrder;
    std::vector<WorkHandle> m_WorkFree;

    std::condition_variable m_StateCV{};

    std::mutex m_StateMutex{}, m_LifeCycleMutex{};
    std::mutex m_WorkMutex{}, m_CycleMutex{};

    int64_t m_SleepMS{1};
public:
    virtual ~ThreadCycler();

    void ValidateThread();

    void Start(int64_t sleepMS);
    void Stop(bool join = true);

    void EnqueueTask(std::function<void()> func);
    template <typename F>
    auto EnqueueFuture(F&& func) {
        std::lock_guard<std::mutex> lock(m_CycleMutex);
        return m_CycleTasks.EnqueueFuture(func);
    }

    WorkHandle CreateWork(VoidJob task, WorkHandle after = UINT32_MAX);
    void RemoveWork(WorkHandle wh);
    void MoveWorkToEnd(WorkHandle wh);

    void AwaitStart();

    bool IsStopped() {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        return m_Stopped;
    }
    bool IsStarted() {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        return m_Started;
    }
    bool IsRunning() const { return m_Running.load(std::memory_order_relaxed); }
protected:
    void DoCycle();
};

template <typename T>
class ThreadContext : public ThreadCycler {
protected:
    WorkHandle m_ConstWorkHandle{UINT32_MAX};
    std::function<void(T& ctx)> m_ConstWork{[](T&){}};

    SharedPtr<T> m_Ctx{nullptr};
    std::mutex m_CtxMutex{};
public:
    using CtxCreatorFunc = std::function<utils::ExResult<SharedPtr<T>>()>;

    bool Start(
        int64_t sleepMS,
        CtxCreatorFunc creatorFunc,
        std::function<void(T& ctx)> constWork = [](T&){}
    ) {
        Start(sleepMS);
        CtxCreatorFunc wrapped = [this, &creatorFunc]() {
            std::lock_guard<std::mutex> lock(m_CtxMutex);
            return creatorFunc();
        };
        auto res = EnqueueFuture(std::move(wrapped)).get();
        if (res.has_error()) {
            std::cerr << res.error().msg << std::endl;
            Stop(true);
            return false;
        }
        m_Ctx = res.value();
        MoveWorkToEnd(m_ConstWorkHandle = CreateWork(constWork));
        return true;
    }

    SharedPtr<T> GetContext() {
        std::lock_guard<std::mutex> lock(m_CtxMutex);
        return m_Ctx;
    }
};

class ThreadContextGeneric : public ThreadContext<EmptyStruct> {
protected:
    std::function<void(EmptyStruct& gctx)> m_SubCycle;
public:
    bool StartCycle(
        int64_t sleepMS,
        std::function<SharedPtr<EmptyStruct>()> initFunc,
        std::function<void(EmptyStruct& gctx)> subCycle
    );
};
// TODO: Create an "ALAudioContext" Class which holds streams, music, sources by respect to audio thread ownership

class ThreadContextWnd : public ThreadContext<IWindow> {
public:
    bool StartApp(CtxCreatorFunc initFunc, int64_t sleepMS = 10);
};

class ThreadContextGfx : public ThreadContext<IRenderContext> {
public:
    bool StartGfx(CtxCreatorFunc initFunc);
};

}