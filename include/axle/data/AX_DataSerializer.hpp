#pragma once

#include "AX_DataStream.hpp"
#include <memory>

namespace axle::data {

class DataSerializer {
protected:
    std::shared_ptr<DataStream> stream;
    uint64_t chunkSize;
public:
    DataSerializer(std::shared_ptr<DataStream> stream, uint64_t chunkSize = 4096);

    uint64_t getLength();
    uint64_t getWritePos();

    void write(const void* bytes, size_t size);

    void skip(uint64_t offset);
    void rewind(uint64_t offset_inv);
};

}