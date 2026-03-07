#pragma once

#include <cstdint>
#include <cstddef>

#include "axle/utils/AX_Expected.hpp"

namespace axle::data {

class IDataStream {
public:
    virtual ~IDataStream() = default;

    virtual utils::ExError Open() = 0;
    virtual bool EndOfStream() const = 0;

    virtual uint64_t GetReadIndex() = 0;
    virtual uint64_t GetWriteIndex() = 0;

    virtual utils::ExError SeekRead(uint64_t pos) = 0;
    virtual utils::ExError SkipRead(int64_t offset) = 0;
    virtual utils::ExError SeekWrite(uint64_t pos) = 0;
    virtual utils::ExError SkipWrite(int64_t offset) = 0;

    virtual utils::ExResult<std::size_t> Read(void* out, std::size_t size) = 0;
    virtual utils::ExResult<std::size_t> Write(const void* in, std::size_t size) = 0;
    virtual utils::ExResult<std::size_t> Write(uint8_t byte, std::size_t repeat) = 0;

    virtual uint64_t GetLength() { return 0; }
};

}