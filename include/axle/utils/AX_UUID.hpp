#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

// AI-Generated

namespace axle::utils
{

class UUID {
public:
    UUID() { bytes.fill(0); }

    static UUID Generate() {
        UUID uuid;

        static thread_local std::random_device rd;
        static thread_local std::mt19937_64 gen(rd());

        std::uniform_int_distribution<uint64_t> dist;

        uint64_t a = dist(gen);
        uint64_t b = dist(gen);

        for (int i = 0; i < 8; i++)
            uuid.bytes[i] = static_cast<uint8_t>((a >> (56 - i * 8)) & 0xFF);

        for (int i = 0; i < 8; i++)
            uuid.bytes[8 + i] = static_cast<uint8_t>((b >> (56 - i * 8)) & 0xFF);

        // RFC4122 version 4
        uuid.bytes[6] = (uuid.bytes[6] & 0x0F) | 0x40;
        uuid.bytes[8] = (uuid.bytes[8] & 0x3F) | 0x80;

        return uuid;
    }

    std::string ToString() const {
        std::ostringstream ss;

        for (int i = 0; i < 16; ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                ss << '-';

            ss << std::hex
               << std::setw(2)
               << std::setfill('0')
               << (int)bytes[i];
        }

        return ss.str();
    }

    bool operator==(const UUID&) const = default;
    auto operator<=>(const UUID&) const = default;

    const std::array<uint8_t, 16>& Data() const { return bytes; }
private:
    std::array<uint8_t, 16> bytes;
};

}

namespace std {

template<>
struct hash<axle::utils::UUID> {
    size_t operator()(const axle::utils::UUID& uuid) const noexcept {
        const auto& d = uuid.Data();

        uint64_t a = 0;
        uint64_t b = 0;

        for (int i = 0; i < 8; ++i)
            a = (a << 8) | d[i];

        for (int i = 8; i < 16; ++i)
            b = (b << 8) | d[i];

        return hash<uint64_t>()(a) ^
               (hash<uint64_t>()(b) << 1);
    }
};

}