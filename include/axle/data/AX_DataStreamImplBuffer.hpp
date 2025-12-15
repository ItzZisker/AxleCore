#pragma once

#include "AX_DataStream.hpp"

#include <filesystem>

namespace axle::data {

class BufferDataStream : public DataStream {
private:
    uint8_t* m_Buffer = nullptr;
    uint64_t m_ReadIndex = 0;
    uint64_t m_WriteIndex = 0;
    uint64_t m_Length = 0;
public:
    BufferDataStream(const uint8_t* buffer, uint64_t length);
    BufferDataStream(uint64_t length);
    ~BufferDataStream() override;

    uint64_t GetReadIndex() override;
    uint64_t GetWriteIndex() override;

    void PeekBytes(void* out, uint64_t pos, std::size_t size);
    void SeekRead(uint64_t pos) override;
    void SkipRead(int64_t offset) override;
    void SeekWrite(uint64_t pos) override;
    void SkipWrite(int64_t offset) override;
    bool EndOfStream() const override;

    std::size_t Read(void* out, std::size_t size) override;
    std::size_t Write(const void* in, std::size_t size) override;

    uint64_t GetLength() override;

    void WriteToFile(uint64_t pos, uint64_t size, const std::filesystem::path& path, uint64_t chunkSize = 4096);
    void WriteToFile(uint64_t size, const std::filesystem::path& path, uint64_t chunkSize = 4096);
    void WriteToFile(const std::filesystem::path& path, uint64_t chunkSize = 4096);
};

}