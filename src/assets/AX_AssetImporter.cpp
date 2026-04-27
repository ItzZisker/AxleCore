#include "axle/assets/AX_AssetImporter.hpp"

namespace axle::assets
{

Metadata::Metadata(MetaType type, utils::SRaw raw)
    : m_Copy(raw), m_Type(type) {}

utils::SRaw& Metadata::Raw() {
    return m_Copy;
}

Metadata::Value Metadata::Get() const {
    const int8_t *raw = m_Copy.data();

    switch (m_Type) {
        case MetaType::Float: { // Float
            float val{};
            if (m_Copy.size() >= sizeof(float)) {
                std::memcpy(&val, raw, sizeof(float));
            }
            return val;
        }
        case MetaType::Double: { // Double
            double val{};
            if (m_Copy.size() >= sizeof(double)) {
                std::memcpy(&val, raw, sizeof(double));
            }
            return val;
        }
        case MetaType::String: { // String
            return std::string((const char*)raw, m_Copy.size());
        }
        case MetaType::Integer: { // Integer
            int val{};
            if (m_Copy.size() >= sizeof(int)) {
                std::memcpy(&val, raw, sizeof(int));
            }
            return val;
        }
        case MetaType::Buffer: { // Buffer
            return m_Copy;
        }
        default: return m_Copy;
    }
}

}