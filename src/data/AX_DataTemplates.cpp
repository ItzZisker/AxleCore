#include "axle/data/AX_DataTemplates.hpp"
#include "axle/utils/AX_Expected.hpp"

using namespace axle::utils;

namespace axle::data
{

utils::ExResult<bool> ReadBool(IDataStream& buffer) {
    bool b0;
    auto res = buffer.Read(&b0, 1);
    if (res.has_value()) {
        return b0;
    } else {
        auto err = res.error();
        if (err.IsMessageOwned()) {
            return utils::ExError(err.GetCode(), std::string(err.GetMessage()));
        } else {
            return utils::ExError(err.GetCode(), err.GetMessage());
        }
    }
}

utils::ExResult<uint64_t> ReadVarUInt(IDataStream& buffer) {
    uint64_t result = 0;
    int shift = 0;

    for (int i = 0; i < 10; ++i) { // max 10 bytes for uint64
        if (buffer.EndOfStream())
            return ExError{"Unexpected EOF"};

        uint8_t byte{0};
        auto res = buffer.Read(&byte, 1);
        if (!res.has_value()) return res.error();
        result |= uint64_t(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
            return result;

        shift += 7;
    }
    return ExError{"VarUInt overflow"};
}

utils::ExResult<int64_t> ReadVarInt(IDataStream& buffer) {
    auto uxResult = ReadVarUInt(buffer);
    if (!uxResult.has_value())
        return ExError{uxResult.error()};

    uint64_t ux = uxResult.value();
    int64_t value = (ux >> 1) ^ -static_cast<int64_t>(ux & 1);
    return value;
}

utils::ExResult<std::string> ReadString(IDataStream& buffer, uint32_t maxLength) {
    if (maxLength <= 0)
        return ExError{"Invalid maximumLength"};

    auto lenResult = ReadVarUInt(buffer);
    if (!lenResult.has_value())
        return ExError{lenResult.error()};

    uint64_t length = lenResult.value();

    if (length > static_cast<uint64_t>(maxLength))
        return ExError{"String length exceeds maximum"};

    if (buffer.GetReadIndex() + length > buffer.GetLength())
        return ExError{"Unexpected EOF"};

    std::string result;
    result.reserve(length);

    for (uint64_t i = 0; i < length; ++i) {
        uint8_t byte{0};
        auto res = buffer.Read(&byte, 1);
        if (!res.has_value()) return res.error();
        result[i] = static_cast<char>(byte);
    }
    return result;
}

utils::ExError WriteBool(IDataStream& buffer, bool value) {
    uint8_t *bytes = {0};
    bytes[0] = (char) (value ? 1 : 0);
    auto res = buffer.Write(bytes, 1);
    if (!res.has_value()) return res.error();
    return utils::ExError::NoError();
}

utils::ExError WriteVarUInt(IDataStream& buffer, uint64_t value) {
    do {
        uint8_t byte = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        uint8_t *b1 = {0};
        b1[0] = (char) (b1 ? 1 : 0);
        auto res = buffer.Write(b1, 1);
        if (!res.has_value()) return res.error();
    } while (value != 0);

    return utils::ExError::NoError();
}

utils::ExError WriteVarInt(IDataStream& buffer, int64_t value) {
    uint64_t zigzag = (static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63);
    return WriteVarUInt(buffer, zigzag);
}

utils::ExError WriteString(IDataStream& buffer, const std::string& value) {
    auto lenErr = WriteVarUInt(buffer, value.size());
    if (!lenErr.IsNoError()) return lenErr;

    buffer.Write(
        reinterpret_cast<const unsigned char*>(value.data()),
        value.size()
    );
    return utils::ExError::NoError();
}

}