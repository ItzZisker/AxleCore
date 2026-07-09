#pragma once

#include "axle/core/window/AX_IWindow.hpp"

#include "axle/graphics/cmd/AX_IGraphicsBackend.hpp"

#include "axle/utils/AX_MagicPool.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>

#define AX_THR_RENDER_OWNED public core::ThreadOwned<core::ThreadContextGfx>
#define AX_THR_WINDOW_OWNED public core::ThreadOwned<core::ThreadContextWnd>

namespace axle::core {

using WorkId = utils::MagicId;

struct WorkHandle : public utils::MagicHandle {};

struct WorkSlot : public utils::MagicInternal<WorkHandle> {
    VoidJob job{};
    bool alive{false};
};

class ThreadCycler {
protected:
    std::atomic_bool m_Running{false};
    bool m_Stopped{false};
    bool m_Started{false};

    std::deque<VoidJob> m_CycleTasks{};
    utils::MagicPool<WorkSlot> m_CycleWorks{utils::MagicPool<WorkSlot>(true)};

    std::thread m_Thread{};
    std::condition_variable m_StateCV{};

    std::mutex m_StateMutex{}, m_LifeCycleMutex{};
    std::mutex m_CycleMutex{}, m_CycleTimeMutex{};

    ChNanos m_CycleTimeCap{ChNanos(0)};

    ChSteadyTimepoint m_LastCycleTimeBegin{ChSteadyClock::now()};
    ChSteadyTimepoint m_LastCycleTimeEnd{ChSteadyClock::now()};
public:
    virtual ~ThreadCycler();

    bool ValidateThread();

    void Start();
    void Stop(bool join = true);

    ThreadId GetThreadId();

    ChNanos GetLastCycleTimeBegin();
    ChNanos GetLastCycleTimeEnd();
    ChNanos GetLastCycleTime(); // delta time

    ChNanos GetCycleTimeCap();
    void SetCycleTimeCap(ChNanos nano_seconds);

    void EnqueueTask(std::function<void()> func);
    template <typename F>
    auto EnqueueFuture(F&& func) {
        std::lock_guard<std::mutex> lock(m_CycleMutex);

        using Ret = decltype(func());

        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<F>(func));
        auto future = task->get_future();
 
        m_CycleTasks.push_back([task]() mutable { (*task)(); });
        return future;
    }

    WorkHandle CreateWork(VoidJob task, WorkId after = UINT32_MAX);
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

template<typename TResult>
class ThreadInvocation {
    using Function = std::function<TResult()>;
private:
    SharedPtr<ThreadCycler> m_Thread;
    Function m_Function;
public:
    template<typename F>
    requires std::invocable<F> &&
             std::same_as<std::invoke_result_t<F>, TResult>
    ThreadInvocation(SharedPtr<ThreadCycler> thread, F&& func)
        : m_Thread(std::move(thread)),
          m_Function(std::forward<F>(func)) {}

    utils::ExResult<TResult> Call() {
        if (m_Thread->ValidateThread()) {
            return m_Function();
        } else {
            return utils::ExError("Invalid caller thread. Call() executes only on the owner thread.");
        }
    }

    TResult SyncCall() {
        if (m_Thread->ValidateThread()) {
            return m_Function();
        } else {
            return thr->EnqueueFuture(m_Function).get();
        }
    }

    Future<TResult> PostCall() {
        if (m_Thread->ValidateThread()) {
            auto task = std::packaged_task<TResult()>(std::forward<Function>(func));
            task();
            return task->get_future();
        } else {
            return m_Thread->EnqueueFuture(m_Function);
        }
    }
};

template<typename F>
inline auto InstaFutureOrQueue(ThreadCycler& thread, F&& func) {
    if (thread.ValidateThread()) {
        using Ret = decltype(func());

        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<F>(func));
        auto future = task->get_future();
        (*task)();
        return future;
    } else {
        return thread.EnqueueFuture(func);
    }
}

template<typename F>
inline void InstaTaskOrQueue(ThreadCycler& thread, const F& func) {
    if (thread.ValidateThread()) {
        func();
    } else {
        thread.EnqueueTask(func);
    }
}

template <typename T>
class ThreadContext : public ThreadCycler {
protected:
    WorkHandle m_ConstWorkHandle{UINT32_MAX};
    std::function<void(T& ctx)> m_ConstWork{[](T&){}};

    SharedPtr<T> m_Ctx{nullptr};
    std::mutex m_CtxMutex{};
public:
    using CtxCreatorFunc = std::function<utils::ExResult<SharedPtr<T>>()>;

    utils::ExError StartSyncd(
        CtxCreatorFunc creator,
        std::function<void()> constWork = [](){}
    ) {
        ThreadCycler::Start();
        ThreadCycler::AwaitStart();

        CtxCreatorFunc wrapped = [this, &creator]() {
            std::lock_guard<std::mutex> lock(m_CtxMutex);
            return creator();
        };

        auto res = EnqueueFuture(std::move(wrapped)).get();
        if (!res.has_value()) {
            Stop(true);
            return {std::string(res.error().GetMessage())};
        }
        m_Ctx = res.value();
    
        MoveWorkToEnd(m_ConstWorkHandle = CreateWork(constWork));
        return utils::ExError::NoError();
    }

    SharedPtr<T> GetContext() {
        std::lock_guard<std::mutex> lock(m_CtxMutex);
        return m_Ctx;
    }
};

template<typename T_ThreadCycler>
requires std::derived_from<T_ThreadCycler, ThreadCycler>
class ThreadOwned {
protected:
    SharedPtr<T_ThreadCycler> m_Thread;
public:
    explicit ThreadOwned(SharedPtr<T_ThreadCycler> eCycler)
        : m_Thread(std::move(eCycler)) {}
protected:
    T_ThreadCycler& Thread() {
        return *m_Thread;
    }

    const T_ThreadCycler& Thread() const {
        return *m_Thread;
    }

    bool OnOwnerThread() const {
        return m_Thread->ValidateThread();
    }

    void AssertOwnerThread() const {
#ifdef AX_DEBUG
        if (!OnOwnerThread())
            throw std::runtime_error("Invalid owner thread.");
#endif
    }
};

class ThreadContextGeneric : public ThreadContext<EmptyStruct> {
protected:
    std::function<void(EmptyStruct& gctx)> m_SubCycle;
public:
    utils::ExError StartCycle(
        std::function<SharedPtr<EmptyStruct>()> initFunc,
        std::function<void(EmptyStruct& gctx)> subCycle
    );
};
// TODO: Create an "ALAudioContext" Class which holds streams, music, sources by respect to audio thread ownership

class ThreadContextWnd : public ThreadContext<core::IWindow> {
public:
    utils::ExError StartApp(CtxCreatorFunc initFunc);
    void SignalWindow();
};

class ThreadContextGfx : public ThreadContext<gfx::IGraphicsBackend> {
private:
    std::atomic_bool m_IsAutoPresent{false};

    std::atomic<float> m_AccumulatedTime{0.0f};
    std::atomic<float> m_FrameTime{0.0f};
    std::atomic<float> m_FrameCap{0.0f};
public:
    utils::ExError StartGfx(CtxCreatorFunc initFunc, float frameCap = 0.0f, bool autoPresent = false);

    bool IsAutoPresent() const { return m_IsAutoPresent.load(); };
    float GetFrameTime() const { return m_FrameTime.load(std::memory_order_relaxed); }
    float GetAccumulatedTime() const { return m_AccumulatedTime.load(std::memory_order_relaxed); }

    void SetAutoPresent(bool autoPresent) { m_IsAutoPresent.store(autoPresent); }
    void SetFrameCap(float frameCap) { m_FrameCap.store(frameCap); }
};

}

namespace axle {
    template <typename T_Thr>
    requires std::is_base_of_v<axle::core::ThreadCycler, T_Thr>
    using ThreadScope = SharedPtr<T_Thr>;

    using ThreadGfxScope = ThreadScope<axle::core::ThreadContextGfx>;
    using ThreadWndScope = ThreadScope<axle::core::ThreadContextWnd>;

    struct VoidInvoke {};

    template <typename F>
    using ThreadInvocation = axle::core::ThreadInvocation<F>;
    using ThreadInvocationVoid = axle::core::ThreadInvocation<VoidInvoke>;
}