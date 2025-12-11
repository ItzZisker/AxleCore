#pragma once

#include "AX_DataStream.hpp"

#include <memory>

namespace axle::data {

class DataSerializer {
protected:
    std::shared_ptr<DataStream> m_Stream;
    uint64_t m_ChunkSize;
public:
    DataSerializer(std::shared_ptr<DataStream> stream, uint64_t chunkSize = 4096);

    uint64_t GetLength();
    uint64_t GetWritePos();

    void Write(const void* bytes, size_t size);

    void Skip(uint64_t offset);
    void Rewind(uint64_t offset_inv);
};

}