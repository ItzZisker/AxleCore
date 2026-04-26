#pragma once

#include "axle/utils/AX_Types.hpp"
#include "axle/utils/AX_Span.hpp"

#include <string>
#include <variant>
#include <utility>
#include <stdexcept>

#define AX_PROPAGATE_ERROR(statement)           \
do {                                            \
    ::axle::utils::ExError __err = (statement); \
    if (!__err.IsNoError()) {                   \
        return __err;                           \
    }                                           \
} while (0)

#define AX_PROPAGATE_RESULT_ERROR(statement)    \
do {                                            \
    auto __res = (statement);                   \
    if (!__res.has_value()) {                   \
        return __res.error();                   \
    }                                           \
} while (0)

#define AX_PROPAGATE_RESULT(statement)    \
do {                                      \
    auto __res = (statement);             \
    if (!__res.has_value()) {             \
        return __res.error();             \
    } else {                              \
        return __res.value();             \
    }                                     \
} while (0)

#define AX_DECL_OR_PROPAGATE(var, expr)  \
    if (!expr.has_value())               \
        return expr.error();             \
    auto var = std::move(expr.value());

#define AX_SET_OR_PROPAGATE(var, expr)   \
    if (!expr.has_value())               \
        return expr.error();             \
    var = std::move(expr.value());

namespace axle::utils
{

class ExError {
private:
    bool m_Owned{false};
    int m_Code{-1};
    SharedPtr<std::string> m_MsgOwned{nullptr};
    std::string_view m_MsgView{"Unknown error"};
public:
    ExError(int code, std::string_view msg) : m_Code(code), m_MsgView(msg) {}
    ExError(std::string_view msg) : m_MsgView(msg) {}

    ExError(int code, const char* msg) : m_Code(code), m_MsgView(msg) {}
    ExError(const char* msg) : m_MsgView(msg) {}

    ExError(int code, std::string msgOwn) : m_Code(code), m_Owned(true) { m_MsgOwned = std::make_shared<std::string>(msgOwn); }
    ExError(std::string msgOwn) : ExError(-1, msgOwn) {}

    int GetCode() const { return m_Code; }
    std::string_view GetMessage() const { return m_Owned ? std::string_view(*m_MsgOwned.get()) : m_MsgView; }

    bool IsMessageOwned() const { return m_Owned; }
    bool IsNoError() const { return m_Code == 0; }
    bool IsValid() const { return !IsNoError(); }
    void ThrowIfValid() const { if (IsValid()) throw std::runtime_error(GetMessage().data()); }

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
        throw std::runtime_error(e.GetMessage().data());
    }
    return std::move(r.value());
}

template<typename T>
inline T ExpectOrThrow(const ExResult<T>& r) {
    if (!r.has_value()) {
        const ExError& e = r.error();
        throw std::runtime_error(e.GetMessage().data());
    }
    return r.value();
}

}
