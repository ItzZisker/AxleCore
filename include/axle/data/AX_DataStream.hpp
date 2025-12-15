#pragma once

#include <cstdint>

namespace axle::data {

class DataStream {
public:
    virtual ~DataStream() = default;

    virtual uint64_t GetReadIndex() = 0;
    virtual uint64_t GetWriteIndex() = 0;

    virtual void SeekRead(uint64_t pos) = 0;
    virtual void SkipRead(int64_t offset) = 0;
    virtual void SeekWrite(uint64_t pos) = 0;
    virtual void SkipWrite(int64_t offset) = 0;
    virtual bool EndOfStream() const = 0;

    virtual std::size_t Read(void* out, std::size_t size) = 0;
    virtual std::size_t Write(const void* in, std::size_t size) = 0;

    virtual uint64_t GetLength() { return 0; }
};

}