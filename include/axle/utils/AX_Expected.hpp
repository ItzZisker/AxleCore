#pragma once

#include <string>
#include <variant>
#include <utility>
#include <stdexcept>

#define AX_PROPAGATE_ERROR(statement)         \
do {                                          \
    ::axle::utils::ExError err = (statement); \
    if (!err.IsNoError()) {                   \
        return err;                           \
    }                                         \
} while (0)

#define AX_PROPAGATE_RESULT_ERROR(statement)  \
do {                                          \
    auto res = (statement);                   \
    if (!res.has_value()) {                   \
        return res.error();                   \
    }                                         \
} while (0)

#define AX_PROPAGATE_RESULT(statement)  \
do {                                    \
    auto res = (statement);             \
    if (!res.has_value()) {             \
        return res.error();             \
    } else {                            \
        return res.value();             \
    }                                   \
} while (0)

namespace axle::utils
{

struct ExError {
    int code{-1};
    std::string msg{"Unknown error"};

    ExError(std::string msg) : msg(msg) {}
    ExError(int code, std::string msg) : code(code), msg(msg) {}

    bool IsNoError() const { return code == 0; }
    bool IsValid() const { return !IsNoError(); }
    void ThrowIfValid() const { if (IsValid()) throw std::runtime_error(msg); }

    static ExError NoError() { return {0, "No error"}; }
};

template<typename T, typename E>
class Expected {
public:
    Expected(const T& value) : m_Data(value) {}
    Expected(T&& value) : m_Data(std::move(value)) {}

    Expected(const E& e) : m_Data(e) {}
    Expected(E&& e) : m_Data(std::move(e)) {}

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
    std::variant<T, E> m_Data;
};

template<typename E>
class Expected<void, E> {
public:
    Expected() : m_Error({}), m_HasValue(true) {}
    Expected(E e) : m_Error(e), m_HasValue(false) {}

    static Expected error(E e) {
        Expected r(std::move(e));
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
using ExResult = Expected<T, ExError>;

template<typename T>
inline ExResult<T> operator|(ExResult<T>&& result, const T&) {
    return std::move(result);
}

template<typename T>
inline ExResult<T> operator|(ExResult<T>&& result, T&&) {
    return std::move(result);
}

template<typename T>
inline ExResult<T> operator|(const ExResult<T>& result, const T&) {
    return result;
}

template<typename T>
inline ExResult<T> operator|(ExResult<T>&& result, const ExError& error_operand) {
    if (!result.has_value()) {
        return result;
    }
    return error_operand; 
}

template<typename T>
inline ExResult<T> operator|(ExResult<T>&& result, ExError&& error_operand) {
    if (!result.has_value()) {
        return std::move(result);
    }
    return std::move(error_operand);
}

template<typename T>
inline ExResult<T> operator|(const ExResult<T>& result, const ExError& error_operand) {
    if (!result.has_value()) {
        return result;
    }
    return error_operand;
}

inline ExError operator|(ExError&& result, const ExError& error_operand) {
    if (!result.IsNoError()) {
        return result;
    }
    return error_operand; 
}

inline ExError operator|(ExError&& result, ExError&& error_operand) {
    if (!result.IsNoError()) {
        return std::move(result);
    }
    return std::move(error_operand);
}

inline ExError operator|(const ExError& result, const ExError& error_operand) {
    if (!result.IsNoError()) {
        return result;
    }
    return error_operand;
}

template<typename T>
inline T ExpectOrThrow(ExResult<T>&& r) {
    if (!r.has_value()) {
        const ExError& e = r.error();
        throw std::runtime_error(e.msg);
    }
    return std::move(r.value());
}

template<typename T>
inline T ExpectOrThrow(const ExResult<T>& r) {
    if (!r.has_value()) {
        const ExError& e = r.error();
        throw std::runtime_error(e.msg);
    }
    return r.value();
}

}
