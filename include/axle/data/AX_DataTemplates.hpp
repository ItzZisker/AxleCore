#pragma once

#include "axle/data/AX_DataSerializer.hpp"
#include "axle/data/AX_DataDeserializer.hpp"

#include "axle/utils/AX_Expected.hpp"

#include <cstdint>
#include <string>

namespace axle::data
{

AX_DECLR_EXRR_FUNC(bool,        ReadBool,     DataDeserializer* buffer);
AX_DECLR_EXRR_FUNC(uint8_t,     ReadChar,     DataDeserializer* buffer);
AX_DECLR_EXRR_FUNC(uint32_t,    ReadVarInt,   DataDeserializer* buffer);
AX_DECLR_EXRR_FUNC(uint64_t,    ReadVarUInt,  DataDeserializer* buffer);
AX_DECLR_EXRR_FUNC(std::string, ReadString,   DataDeserializer* buffer,  uint32_t maximumLength = 1024);

AX_DECLR_EXRR_FUNC(void,    WriteBool,     DataSerializer* buffer,  bool value);
AX_DECLR_EXRR_FUNC(void,    WriteChar,     DataSerializer* buffer,  char value);
AX_DECLR_EXRR_FUNC(void,    WriteVarInt,   DataSerializer* buffer,  int64_t value);
AX_DECLR_EXRR_FUNC(void,    WriteVarUInt,  DataSerializer* buffer,  uint64_t value);
AX_DECLR_EXRR_FUNC(void,    WriteString,   DataSerializer* buffer,  const std::string& value, uint32_t maximumLength = 1024);

}