#pragma once

#include "AX_DataStream.hpp"

#include <fstream>
#include <filesystem>

namespace axle::data {

class FileDataStream : public DataStream {
private:
    std::fstream file;
    bool readable = false;
    bool writable = false;
    uint64_t cachedLength = 0;
public:
    FileDataStream(const std::filesystem::path& path, bool read, bool write);
    ~FileDataStream() override;

    uint64_t getReadIndex() override;
    uint64_t getWriteIndex() override;

    void seekRead(uint64_t pos) override;
    void skipRead(int64_t offset) override;
    void seekWrite(uint64_t pos) override;
    void skipWrite(int64_t offset) override;
    bool endOfStream() const override;

    size_t read(void* out, size_t size) override;
    size_t write(const void* in, size_t size) override;

    uint64_t getLength() override;
};

}