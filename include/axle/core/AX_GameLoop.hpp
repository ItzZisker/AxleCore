#pragma once

#include "axle/core/window/AX_IWindow.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/concurrency/AX_TaskQueue.hpp"
#include "axle/utils/AX_Types.hpp"

#include <condition_variable>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <map>

#define AX_WKKEY_CONSTANT "__ConstantWork__"

namespace axle::core {

class WorkOrder {
    OrderPredicate m_Order;
public:
    WorkOrder(OrderPredicate pred = std::less<>{}) : m_Order(std::move(pred)) {}

    bool operator()(const std::string& a, const std::string& b) const {
        if (a == AX_WKKEY_CONSTANT) return false;
        if (b == AX_WKKEY_CONSTANT) return true;
        return m_Order(a, b);
    }
};

class ThreadCycler {
public:
    ThreadCycler(WorkOrder order = {});
    virtual ~ThreadCycler();

    void ValidateThread();

    void Start(int64_t msSleep, std::function<void()> initFunc);
    void Stop(bool join = true);

    void EnqueueTask(std::function<void()> func);
    template <typename F>
    auto EnqueueFuture(F&& func) {
        std::lock_guard<std::mutex> lock(m_CycleMutex);
        return m_CycleTasks->EnqueueFuture(func);
    }

    void PushWork(const std::string& key, std::function<void()> func);
    void PopWork(const std::string& key);

    void SetWorkOrder(WorkOrder order = {});

    void AwaitStart();
protected:
    std::atomic_bool m_Running{false};
    bool m_Stopped{false};
    bool m_Started{false};

    UniquePtr<TaskQueue> m_CycleTasks{nullptr};

    std::thread m_Thread{};

    std::map<std::string, VoidJob, WorkOrder> m_Works;

    std::condition_variable m_StateCV{};

    std::mutex m_StateMutex{}, m_InitMutex{}, m_LifeCycleMutex{};
    std::mutex m_WorkMutex{}, m_CycleMutex{};

    int64_t m_msSleep{1};

    void DoCycle();
};

template <typename T>
class ThreadContext : public ThreadCycler {
protected:
    SharedPtr<T> m_Ctx{nullptr};
public:
    void Start(int64_t msSleep, std::function<SharedPtr<T>()> creatorFunc) {
        Start(msSleep, [this, crtFunc = std::move(creatorFunc)](){
            m_Ctx = crtFunc();
        });
    }

    std::shared_ptr<T> GetContext() {
        std::lock_guard<std::mutex> lock(m_InitMutex);
        return m_Ctx;
    }
};

class ThreadContextGeneric : public ThreadContext<EmptyStruct> {
protected:
    std::function<void(ThreadContextGeneric& gctx)> m_SubCycle;
public:
    void StartCycle(
        int64_t mssleep,
        std::function<SharedPtr<EmptyStruct>()> initFunc,
        std::function<void(ThreadContextGeneric& gctx)> subCycle
    );
};
// TODO: Create an "ALAudioContext" Class which holds streams, music, sources by respect to audio thread ownership

class ThreadContextWnd : public ThreadContext<IWindow> {
public:
    void StartApp(std::function<SharedPtr<IWindow>()> initFunc);
};

class ThreadContextGfx : public ThreadContext<IRenderContext> {
public:
    void StartGfx(std::function<SharedPtr<IRenderContext>()> initFunc);
};

}