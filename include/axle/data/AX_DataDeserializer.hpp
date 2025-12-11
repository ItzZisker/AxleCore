#pragma once

#include "AX_DataStream.hpp"

#include <memory>
#include <cstdint>

namespace axle::data {

class DataDeserializer {
protected:
    std::shared_ptr<DataStream> stream;
    uint64_t chunkSize;
public:
    DataDeserializer(std::shared_ptr<DataStream> stream, uint64_t chunkSize = 4096);

    uint64_t getLength();
    uint64_t getReadPos();

    uint8_t readByte();
    void read(void* out, size_t size);

    void skip(uint64_t offset);
    void rewind(uint64_t offset_inv);
};

}