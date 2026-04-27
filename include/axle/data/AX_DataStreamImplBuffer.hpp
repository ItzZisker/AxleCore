#pragma once

#include "AX_IDataStream.hpp"

#include "axle/utils/AX_Span.hpp"
#include "axle/utils/AX_Expected.hpp"

#include <filesystem>
#include <vector>

namespace axle::data {

class BufferDataStream : public IDataStream {
private:
    utils::URawView m_BufferView{nullptr, 0};
    std::vector<uint8_t> m_BufferHeld;

    uint64_t m_ReadIndex = 0;
    uint64_t m_WriteIndex = 0;

    bool m_Opened{false};
public:
    BufferDataStream(utils::URawView bufferView); // Held an object view (Span) refers to data (no ownership)
    BufferDataStream(uint64_t length); // Creates and owns data itself
    ~BufferDataStream() override;

    utils::ExError Open() override;
    bool EndOfStream() const override;

    uint64_t GetReadIndex() override;
    uint64_t GetWriteIndex() override;

    utils::ExError PeekBytes(void* out, uint64_t pos, std::size_t size);

    utils::ExError SeekRead(uint64_t pos) override;
    utils::ExError SkipRead(int64_t offset) override;
    utils::ExError SeekWrite(uint64_t pos) override;
    utils::ExError SkipWrite(int64_t offset) override;

    utils::ExResult<std::size_t> Read(void* out, std::size_t size) override;
    utils::ExResult<std::size_t> Write(const void* in, std::size_t size) override;
    utils::ExResult<std::size_t> Write(uint8_t byte, std::size_t repeat) override;

    uint64_t GetLength() override;

    utils::ExError WriteToFile(uint64_t pos, uint64_t size, const std::filesystem::path& path, uint64_t chunkSize = 4096);
    utils::ExError WriteToFile(uint64_t size, const std::filesystem::path& path, uint64_t chunkSize = 4096);
    utils::ExError WriteToFile(const std::filesystem::path& path, uint64_t chunkSize = 4096);
};

}