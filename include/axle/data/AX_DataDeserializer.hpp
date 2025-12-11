#pragma once

#include "AX_DataStream.hpp"

#include <memory>
#include <cstdint>

namespace axle::data {

class DataDeserializer {
protected:
    std::shared_ptr<DataStream> m_Stream;
    uint64_t m_ChunkSize;
public:
    DataDeserializer(std::shared_ptr<DataStream> stream, uint64_t chunkSize = 4096);

    uint64_t GetLength();
    uint64_t GetReadPos();

    uint8_t ReadByte();
    void Read(void* out, size_t size);

    void Skip(uint64_t offset);
    void Rewind(uint64_t offset_inv);
};

}