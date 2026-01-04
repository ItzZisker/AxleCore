#pragma once

#include <functional>
#include <future>
#include <thread>
#include <deque>

namespace axle::core {

using VoidJob = std::function<void()>;

namespace future {
    template <typename T, typename F>
    void when_complete(std::future<T>&& fut, F&& callback) {
        std::thread([fut = std::move(fut), callback = std::forward<F>(callback)]() mutable {
            try {
                T value = fut.get(); // wait and get result
                callback(std::move(value), std::exception_ptr{});
            } catch (...) {
                callback(T{}, std::current_exception());
            }
        }).detach();
    }
}

class TaskQueue {
public:
    void ExecuteAll();
    void Enqueue(VoidJob task);

    std::deque<VoidJob> CopyJobs();
    
    template<typename F>
    auto EnqueueFuture(F&& f) {
        using Ret = decltype(f());

        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<F>(f));
        auto future = task->get_future();
 
        enqueue([task]() mutable {
            (*task)();
        });
        return future;
    }
private:
    std::deque<VoidJob> m_Tasks{};
    std::mutex m_Mutex{};
};

}