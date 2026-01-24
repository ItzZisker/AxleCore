#pragma once

#include <functional>
#include <future>
#include <memory>

namespace axle
{

template <typename T>
using Job = std::function<T()>;
using VoidJob = Job<void>;

template <typename T>
using Future = std::future<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;
template <typename T>
using UniquePtr = std::unique_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;

}