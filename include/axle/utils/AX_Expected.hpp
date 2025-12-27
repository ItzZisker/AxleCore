#pragma once

#include <string>
#include <variant>
#include <utility>
#include <stdexcept>

namespace axle::utils
{

#define AX_EXRR_FUNC(ReturnType, Name, ...) \
    utils::ExResult<ReturnType> Name(__VA_ARGS__)

class ExException : public std::runtime_error {
public:
    int code;
    ExException(int code, const std::string& msg)
        : std::runtime_error(msg), code(code) {}
};

struct AXError {
    int code = 0;
    std::string msg = "Unknown error";

    AXError(std::string msg) : msg(msg) {}
    AXError(int code, std::string msg) : code(code), msg(msg) {}
};

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

template<typename E>
class Expected<void, E> {
public:
    Expected() : m_HasValue(true) {}
    static Expected error(E e) {
        Expected r;
        r.m_HasValue = false;
        r.m_Error = std::move(e);
        return r;
    }

    bool has_value() const { return m_HasValue; }

    E& error() {
        if (m_HasValue) throw std::runtime_error("Accessed error, but has value");
        return m_Error;
    }

    const E& error() const {
        if (m_HasValue) throw std::runtime_error("Accessed error, but has value");
        return m_Error;
    }
private:
    bool m_HasValue = false;
    E m_Error;
};

template<typename T>
using ExResult = Expected<T, AXError>;

template<typename T>
utils::Expected<T, AXError> RExError(std::string msg) {
    return utils::Expected<T, AXError>::error({std::move(msg)});
}

template<typename T>
utils::Expected<T, AXError> RExError(int code, std::string msg) {
    return utils::Expected<T, AXError>::error({std::move(code), std::move(msg)});
}

template<typename T>
utils::Expected<T, AXError> RExError(const AXError& ref) {
    return utils::Expected<T, AXError>::error({ref});
}

template<typename T>
T ExpectOrThrow(ExResult<T>&& r) {
    if (!r.has_value()) {
        const AXError& e = r.error();
        throw ExException(e.code, e.msg);
    }
    return std::move(r.value());
}

template<typename T>
T ExpectOrThrow(const ExResult<T>& r) {
    if (!r.has_value()) {
        const AXError& e = r.error();
        throw ExException(e.code, e.msg);
    }
    return r.value();
}

}
