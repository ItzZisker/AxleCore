#pragma once

#include <functional>
#include <future>
#include <deque>

namespace axle::core {

using VoidJob = std::function<void()>;

class TaskQueue {
public:
    void ExecuteAll();
    void Enqueue(VoidJob task);

    std::deque<VoidJob> CopyJobs();
    std::deque<VoidJob> MoveJobs();
    
    template<typename F>
    auto EnqueueFuture(F&& f) {
        using Ret = decltype(f());
    
        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<F>(f));
        auto future = task->get_future();
 
        Enqueue([task]() mutable {
            (*task)();
        });
        return future;
    }
private:
    std::deque<VoidJob> m_Tasks{};
    std::mutex m_Mutex{};
};

}