#pragma once

#include "axle/data/AX_IDataStream.hpp"

#include "axle/utils/AX_Expected.hpp"

#include "glm/glm.hpp"

#include <cstdint>
#include <string>

namespace axle::data
{

utils::ExResult<glm::mat4> LE_ReadMat4(IDataStream& buffer);
utils::ExResult<glm::mat3> LE_ReadMat3(IDataStream& buffer);

utils::ExResult<glm::vec4> LE_ReadVec4(IDataStream& buffer);
utils::ExResult<glm::vec3> LE_ReadVec3(IDataStream& buffer);
utils::ExResult<glm::vec2> LE_ReadVec2(IDataStream& buffer);

utils::ExResult<glm::ivec4> LE_ReadIVec4(IDataStream& buffer);
utils::ExResult<glm::ivec3> LE_ReadIVec3(IDataStream& buffer);
utils::ExResult<glm::ivec2> LE_ReadIVec2(IDataStream& buffer);

utils::ExResult<bool> ReadBool(IDataStream& buffer);
utils::ExResult<uint8_t> ReadChar(IDataStream& buffer);
utils::ExResult<int64_t> ReadVarInt(IDataStream& buffer);
utils::ExResult<uint64_t> ReadVarUInt(IDataStream& buffer);
utils::ExResult<std::string> ReadString(IDataStream& buffer, uint32_t maxLength = 1024);

utils::ExError LE_WriteMat4(IDataStream& buffer, const glm::mat4& value);
utils::ExError LE_WriteMat3(IDataStream& buffer, const glm::mat3& value);

utils::ExError LE_WriteVec4(IDataStream& buffer, const glm::vec4& value);
utils::ExError LE_WriteVec3(IDataStream& buffer, const glm::vec3& value);
utils::ExError LE_WriteVec2(IDataStream& buffer, const glm::vec2& value);

utils::ExError LE_WriteIVec4(IDataStream& buffer, const glm::ivec4& value);
utils::ExError LE_WriteIVec3(IDataStream& buffer, const glm::ivec3& value);
utils::ExError LE_WriteIVec2(IDataStream& buffer, const glm::ivec2& value);

utils::ExError WriteBool(IDataStream& buffer, bool value);
utils::ExError WriteChar(IDataStream& buffer, uint8_t value);
utils::ExError WriteVarInt(IDataStream& buffer, int64_t value);
utils::ExError WriteVarUInt(IDataStream& buffer, uint64_t value);
utils::ExError WriteString(IDataStream& buffer, const std::string& value, uint32_t maxLength = 1024);

}