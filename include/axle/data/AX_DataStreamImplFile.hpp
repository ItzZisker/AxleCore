#pragma once

#include "AX_IDataStream.hpp"

#include <filesystem>
#include <fstream>

namespace axle::data {

class FileDataStream : public IDataStream {
private:
    std::filesystem::path m_Path;
    std::fstream m_File;

    bool m_Readable{false};
    bool m_Writable{false};

    bool m_Opened{false};
public:
    FileDataStream(const std::filesystem::path& path, bool read, bool write);
    ~FileDataStream() override;

    utils::ExError Open() override;
    bool EndOfStream() const override;

    uint64_t GetReadIndex() override;
    uint64_t GetWriteIndex() override;

    utils::ExError SeekRead(uint64_t pos) override;
    utils::ExError SkipRead(int64_t offset) override;
    utils::ExError SeekWrite(uint64_t pos) override;
    utils::ExError SkipWrite(int64_t offset) override;

    utils::ExResult<std::size_t> Read(void* out, std::size_t size) override;
    utils::ExResult<std::size_t> Write(const void* in, std::size_t size) override;
    utils::ExResult<std::size_t> Write(uint8_t byte, std::size_t repeat) override;

    uint64_t GetLength() override;
};

}