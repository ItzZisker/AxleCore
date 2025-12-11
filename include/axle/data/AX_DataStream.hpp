#pragma once

#include <cstdint>

namespace axle::data {

class DataStream {
public:
    virtual ~DataStream() = default;

    virtual uint64_t getReadIndex() = 0;
    virtual uint64_t getWriteIndex() = 0;

    virtual void seekRead(uint64_t pos) = 0;
    virtual void skipRead(int64_t offset) = 0;
    virtual void seekWrite(uint64_t pos) = 0;
    virtual void skipWrite(int64_t offset) = 0;
    virtual bool endOfStream() const = 0;

    virtual size_t read(void* out, size_t size) = 0;
    virtual size_t write(const void* in, size_t size) = 0;

    virtual uint64_t getLength() { return 0; }
};

}