#include "axle/data/AX_DataTemplates.hpp"
#include "axle/utils/AX_Expected.hpp"

using namespace axle::utils;

namespace axle::data
{

AX_EXRR_FUNC(bool, ReadBool, DataDeserializer *buffer) {
    if (!buffer) return RExError<bool>("null buffer");
    return buffer->ReadByte();
}

AX_EXRR_FUNC(uint64_t, ReadVarUInt64, DataDeserializer* buffer) {
    if (!buffer)
        return RExError<uint64_t>("null buffer");

    uint64_t result = 0;
    int shift = 0;

    for (int i = 0; i < 10; ++i) { // max 10 bytes for uint64
        if (buffer->IsEndOfStream())
            return RExError<uint64_t>("EndOfStream");

        uint8_t byte = buffer->ReadByte();
        result |= uint64_t(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
            return result;

        shift += 7;
    }
    return RExError<uint64_t>("VarUInt64 overflow");
}

AX_EXRR_FUNC(int64_t, ReadVarInt64, DataDeserializer* buffer) {
    if (!buffer)
        return RExError<int64_t>("null buffer");

    auto uxResult = ReadVarUInt64(buffer);
    if (!uxResult.has_value())
        return RExError<int64_t>(uxResult.error());

    uint64_t ux = uxResult.value();
    int64_t value = (ux >> 1) ^ -static_cast<int64_t>(ux & 1);
    return value;
}

AX_EXRR_FUNC(std::string, ReadString, DataDeserializer* buffer, int maximumLength) {
    if (!buffer)
        return RExError<std::string>("null buffer");

    if (maximumLength <= 0)
        return RExError<std::string>("invalid maximumLength");

    auto lenResult = ReadVarUInt64(buffer);
    if (!lenResult.has_value())
        return RExError<std::string>(lenResult.error());

    uint64_t length = lenResult.value();

    if (length > static_cast<uint64_t>(maximumLength))
        return RExError<std::string>("String length exceeds maximum");

    if (buffer->GetReadPos() + length > buffer->GetLength())
        return RExError<std::string>("EndOfStream");

    std::string result;
    result.reserve(length);

    for (uint64_t i = 0; i < length; ++i) {
        result.push_back(static_cast<char>(buffer->ReadByte()));
    }
    return result;
}

AX_EXRR_FUNC(void, WriteBool, DataSerializer* buffer, bool value) {
    if (!buffer)
        return RExError<void>("null buffer");

    uint8_t *bytes = {0};
    bytes[0] = (char) (value ? 1 : 0);
    buffer->Write(bytes, 1);
    return {};
}

AX_EXRR_FUNC(void, WriteVarUInt64, DataSerializer* buffer, uint64_t value) {
    if (!buffer)
        return RExError<void>("null buffer");

    do {
        uint8_t byte = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;
        if (value != 0)
            byte |= 0x80;
        uint8_t *b1 = {0};
        b1[0] = (char) (b1 ? 1 : 0);
        buffer->Write(b1, 1);
    } while (value != 0);

    return {};
}

AX_EXRR_FUNC(void, WriteVarInt64, DataSerializer* buffer, int64_t value) {
    if (!buffer)
        return RExError<void>("null buffer");

    uint64_t zigzag =
        (static_cast<uint64_t>(value) << 1) ^
        static_cast<uint64_t>(value >> 63);

    return WriteVarUInt64(buffer, zigzag);
}

AX_EXRR_FUNC(void, WriteString, DataSerializer* buffer, const std::string& value) {
    if (!buffer)
        return RExError<void>("null buffer");

    auto lenResult = WriteVarUInt64(buffer, value.size());
    if (!lenResult.has_value())
        return lenResult;

    buffer->Write(
        reinterpret_cast<const unsigned char*>(value.data()),
        value.size()
    );
    return {};
}

}