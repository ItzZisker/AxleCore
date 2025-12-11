#include "axle/data/AX_LittleEndian.hpp"

namespace axle::data
{

template<typename T>
void LE_write(DataSerializer* buff, T value) {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    std::reverse(bytes, bytes + sizeof(T));
#endif
    buff->Write(reinterpret_cast<unsigned char*>(bytes), sizeof(T));
}

template<typename T>
T LE_read(DataDeserializer* buff) {
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
    uint8_t bytes[sizeof(T)];
    buff->Read(reinterpret_cast<unsigned char*>(bytes), sizeof(T));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    std::reverse(bytes, bytes + sizeof(T));
#endif
    T value{};
    std::memcpy(&value, bytes, sizeof(T));
    return value;
}

}