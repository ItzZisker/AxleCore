#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>

#define AX_NON_COPYABLE_NON_MOVABLE(Type)    \
    Type(const Type&) = delete;              \
    Type& operator=(const Type&) = delete;   \
    Type(Type&&) = delete;                   \
    Type& operator=(Type&&) = delete;

namespace axle
{

struct EmptyStruct {};

using ChSteadyClock = std::chrono::steady_clock;

using ChMillis = std::chrono::milliseconds;
using ChNanos = std::chrono::nanoseconds;

using ChSteadyClock = std::chrono::steady_clock;
using ChSteadyTimepoint = std::chrono::time_point<ChSteadyClock>;

template <typename T>
using Job = std::function<T()>;
using VoidJob = Job<void>;

using TickJob = std::function<void(float dTSeconds)>;

template <typename T>
using Predicate = std::function<bool(const T& a, const T& b)>;

template <typename T>
using Future = std::future<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;
template <typename T>
using UniquePtr = std::unique_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T>
using Scope = SharedPtr<T>;
template <typename T>
using View = WeakPtr<T>;

using ThreadId = std::thread::id;

}