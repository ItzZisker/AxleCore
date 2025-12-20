#pragma once

#include <variant>
#include <utility>
#include <stdexcept>

namespace axle::utils
{

template<typename T, typename E>
class Expected {
public:
    Expected(const T& value) : m_Data(value) {}
    Expected(T&& value) : m_Data(std::move(value)) {}

    static Expected<T, E> error(const E& e) { return Expected(e); }
    static Expected<T, E> error(E&& e) { return Expected(std::move(e)); }

    bool has_value() const { return std::holds_alternative<T>(m_Data); }

    T& value() {
        if (!has_value()) throw std::runtime_error("Accessed value, but it's an error");
        return std::get<T>(m_Data);
    }

    const T& value() const {
        if (!has_value()) throw std::runtime_error("Accessed value, but it's an error");
        return std::get<T>(m_Data);
    }

    E& error() {
        if (has_value()) throw std::runtime_error("Accessed error, but it's a value");
        return std::get<E>(m_Data);
    }

    const E& error() const {
        if (has_value()) throw std::runtime_error("Accessed error, but it's a value");
        return std::get<E>(m_Data);
    }
private:
    Expected(const E& e) : m_Data(e) {}
    Expected(E&& e) : m_Data(std::move(e)) {}

    std::variant<T, E> m_Data;
};

}
