#pragma once

#include <array>
#include <vector>

namespace axle::utils
{

template<typename T>
class Span {
public:
    using iterator = std::add_pointer_t<T>;
    using const_iterator = const T*;

    constexpr Span() noexcept : m_ptr(nullptr), m_size(0) {}

    constexpr Span(T* ptr, std::size_t size) noexcept
        : m_ptr(ptr), m_size(size) {}

    T& operator[](std::size_t index) noexcept {
        return m_ptr[index];
    }

    const T& operator[](std::size_t index) const noexcept {
        return m_ptr[index];
    }

    constexpr iterator begin() const noexcept { return m_ptr; }
    constexpr iterator end() const noexcept { return m_ptr + m_size; }

    constexpr T* handle() const noexcept { return m_ptr; }
    constexpr std::size_t size() const noexcept { return m_size; }
private:
    T* m_ptr;
    std::size_t m_size;
};

template<typename T>
class CowSpan {
private:
    bool m_Owned{false};

    Span<T> m_View{};
    std::vector<T> m_Storage{};
public:
    using iterator = T*;
    using const_iterator = const T*;

    CowSpan() = default;
    CowSpan(Span<T> span) : m_Owned(false), m_View(span) {}
    CowSpan(std::vector<T> data) : m_Owned(true) {
        m_Storage = std::vector<T>(std::move(data));
        m_View = Span<T>(m_Storage.data(), m_Storage.size());
    }

    CowSpan(const CowSpan& other) : m_Owned(other.m_Owned), m_Storage(other.m_Storage) {
        if (m_Owned) {
            m_View = Span<T>(m_Storage.data(), m_Storage.size());
        } else {
            m_View = other.m_View;
        }
    }

    CowSpan& operator=(const CowSpan& other) {
        if (this == &other)
            return *this;

        m_Owned = other.m_Owned;
        m_Storage = other.m_Storage;

        if (m_Owned) {
            m_View = Span<T>(m_Storage.data(), m_Storage.size());
        } else {
            m_View = other.m_View;
        }
        return *this;
    }

    CowSpan(CowSpan&& other) noexcept : m_Owned(other.m_Owned), m_Storage(std::move(other.m_Storage)) {
        if (m_Owned) {
            m_View = Span<T>(m_Storage.data(), m_Storage.size());
        } else {
            m_View = other.m_View;
        }
    }

    T& operator[](std::size_t index) noexcept { return m_View[index]; }
    const T& operator[](std::size_t index) const noexcept { return m_View[index]; }

    const Span<T>& Get() const { return m_View; }

    T* data() const { return Get().handle(); }
    size_t size() const { return Get().size(); }

    iterator begin() { return data(); }
    iterator end() { return data() + size(); }

    const_iterator begin() const { return data(); }
    const_iterator end() const { return data() + size(); }
};

}