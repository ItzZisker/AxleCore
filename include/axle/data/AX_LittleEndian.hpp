#pragma once

#include "axle/data/AX_DataDeserializer.hpp"
#include "axle/data/AX_DataSerializer.hpp"

#include <bit>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace axle::data {

template<typename T>
inline bool LE_Write(DataSerializer& buff, T value) {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));

    if constexpr (std::endian::native == std::endian::big) {
        std::reverse(bytes, bytes + sizeof(T));
    }

    buff.Write(bytes, sizeof(T));
    return true;
}

template<typename T>
inline void LE_WriteOrThrow(DataSerializer& buff, T value) {
    if (!LE_Write(buff, value)) {
        throw std::runtime_error("LE_WriteOrThrow(): LE_Write() Failed");
    }
}

template<typename T>
inline bool LE_Read(DataDeserializer& buff, T& outValue) {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

    if (buff.GetReadPos() + sizeof(T) > buff.GetLength())
        return false;

    uint8_t bytes[sizeof(T)];
    buff.Read(bytes, sizeof(T));

    if constexpr (std::endian::native == std::endian::big) {
        std::reverse(bytes, bytes + sizeof(T));
    }

    std::memcpy(&outValue, bytes, sizeof(T));
    return true;
}

template<typename T>
inline T LE_ReadOrThrow(DataDeserializer& buff) {
    T value;
    bool res = LE_Read(buff, value);
    if (!res) throw std::runtime_error("LE_ReadOrThrow(): LE_Read() Failed");
    return value;
}

}
