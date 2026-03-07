#pragma once

#include <array>

namespace axle::utils
{

template<typename T>
class Span {
public:
    Span() noexcept : m_ptr(nullptr), m_size(0) {}

    constexpr Span(T* ptr, std::size_t size) noexcept
        : m_ptr(ptr), m_size(size) {}

    T& operator[](std::size_t index) const noexcept {
        return m_ptr[index];
    }

    using iterator = std::add_pointer_t<T>;

    constexpr iterator begin() const noexcept {
        return m_ptr;
    }
    constexpr iterator end() const noexcept {
        return m_ptr + m_size;
    }

    constexpr T* handle() const noexcept {
        return m_ptr;
    }
    constexpr std::size_t size() const noexcept {
        return m_size;
    }
private:
    T* m_ptr;
    std::size_t m_size;
};

}