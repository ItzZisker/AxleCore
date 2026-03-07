#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <bit>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace axle::data {

template<typename T>
inline utils::ExResult<std::size_t> LE_Write(IDataStream& buff, T* valueAddr) {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, valueAddr, sizeof(T));

    if constexpr (std::endian::native == std::endian::big) {
        std::reverse(bytes, bytes + sizeof(T));
    }
    AX_PROPAGATE_RESULT(buff.Write(bytes, sizeof(T)));
}

template<typename T>
inline utils::ExResult<std::size_t> LE_Read(IDataStream& buffer, T* out_valueAddr) {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

    uint8_t bytes[sizeof(T)];
    auto res = buffer.Read(bytes, sizeof(T));

    if (!res.has_value()) {
        return res.error();
    }
    if constexpr (std::endian::native == std::endian::big) {
        std::reverse(bytes, bytes + sizeof(T));
    }

    std::memcpy(out_valueAddr, bytes, sizeof(T));
    return res.value();
}

}
