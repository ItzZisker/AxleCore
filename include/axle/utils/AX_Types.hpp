#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>

namespace axle
{

struct EmptyStruct {};

using ChMillis = std::chrono::milliseconds;

template <typename T>
using Job = std::function<T()>;
using VoidJob = Job<void>;

using TickJob = std::function<void(float dTSeconds)>;

using OrderPredicate = std::function<bool(const std::string& a, const std::string& b)>;

template <typename T>
using Future = std::future<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;
template <typename T>
using UniquePtr = std::unique_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;

}