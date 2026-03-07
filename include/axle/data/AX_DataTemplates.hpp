#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <cstdint>
#include <string>

namespace axle::data
{

utils::ExResult<bool> ReadBool(IDataStream& buffer);
utils::ExResult<uint8_t> ReadChar(IDataStream& buffer);
utils::ExResult<int64_t> ReadVarInt(IDataStream& buffer);
utils::ExResult<uint64_t> ReadVarUInt(IDataStream& buffer);
utils::ExResult<std::string> ReadString(IDataStream& buffer, uint32_t maxLength = 1024);

utils::ExError WriteBool(IDataStream& buffer, bool value);
utils::ExError WriteChar(IDataStream& buffer, uint8_t value);
utils::ExError WriteVarInt(IDataStream& buffer, int64_t value);
utils::ExError WriteVarUInt(IDataStream& buffer, uint64_t value);
utils::ExError WriteString(IDataStream& buffer, const std::string& value, uint32_t maxLength = 1024);

}