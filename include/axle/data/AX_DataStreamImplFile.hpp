#pragma once

#include "AX_IDataStream.hpp"

#include <filesystem>
#include <fstream>

namespace axle::data {

class FileDataStream : public IDataStream {
private:
    std::fstream m_File;
    uint64_t m_CachedLength = 0;
    bool m_Readable = false;
    bool m_Writable = false;
public:
    FileDataStream(const std::filesystem::path& path, bool read, bool write);
    ~FileDataStream() override;

    uint64_t GetReadIndex() override;
    uint64_t GetWriteIndex() override;

    void SeekRead(uint64_t pos) override;
    void SkipRead(int64_t offset) override;
    void SeekWrite(uint64_t pos) override;
    void SkipWrite(int64_t offset) override;
    bool EndOfStream() const override;

    std::size_t Read(void* out, std::size_t size) override;
    std::size_t Write(const void* in, std::size_t size) override;

    uint64_t GetLength() override;
};

}