#pragma once

#include "AX_DataStream.hpp"

#include <memory>
#include <filesystem>

namespace axle::data {

class DataStreamImplBuffer : public DataStream {
private:
    uint8_t* buffer = nullptr;
    uint64_t readIndex = 0;
    uint64_t writeIndex = 0;
    uint64_t length = 0;
public:
    DataStreamImplBuffer(const uint8_t* buffer, uint64_t length);
    DataStreamImplBuffer(uint64_t length);
    ~DataStreamImplBuffer() override;

    uint64_t getReadIndex() override;
    uint64_t getWriteIndex() override;

    void peekBytes(void* out, uint64_t pos, size_t size);
    void seekRead(uint64_t pos) override;
    void skipRead(int64_t offset) override;
    void seekWrite(uint64_t pos) override;
    void skipWrite(int64_t offset) override;
    bool endOfStream() const override;

    size_t read(void* out, size_t size) override;
    size_t write(const void* in, size_t size) override;

    uint64_t getLength() override;
};

using BufferDataStream = DataStreamImplBuffer;

void writeBufferStreamToFile(std::shared_ptr<DataStreamImplBuffer> stream, uint64_t pos, uint64_t size, const std::filesystem::path& path, uint64_t chunkSize = 4096);
void writeBufferStreamToFile(std::shared_ptr<DataStreamImplBuffer> stream, uint64_t size, const std::filesystem::path& path, uint64_t chunkSize = 4096);
void writeBufferStreamToFile(std::shared_ptr<DataStreamImplBuffer> stream, const std::filesystem::path& path, uint64_t chunkSize = 4096);

}