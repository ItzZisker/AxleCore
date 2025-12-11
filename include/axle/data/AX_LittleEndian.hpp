#pragma once

#include "AX_DataSerializer.hpp"
#include "AX_DataDeserializer.hpp"

#include <cstring>

namespace axle::data::LittleEndian
{
    template<typename T>
    void write(DataSerializer* buff, T value) {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        uint8_t bytes[sizeof(T)];
        std::memcpy(bytes, &value, sizeof(T));
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        std::reverse(bytes, bytes + sizeof(T));
    #endif
        buff->write(reinterpret_cast<unsigned char*>(bytes), sizeof(T));
    }

    template<typename T>
    T read(DataDeserializer* buff) {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        uint8_t bytes[sizeof(T)];
        buff->read(reinterpret_cast<unsigned char*>(bytes), sizeof(T));
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        std::reverse(bytes, bytes + sizeof(T));
    #endif
        T value{};
        std::memcpy(&value, bytes, sizeof(T));
        return value;
    }
}