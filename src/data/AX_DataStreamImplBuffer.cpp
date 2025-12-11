#include "axle/data/AX_DataStreamImplBuffer.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace axle::data
{

BufferDataStream::BufferDataStream(const uint8_t* buffer, uint64_t length) : m_Length(length) {
    this->m_Buffer = new uint8_t[length];
    if (buffer) std::memcpy(this->m_Buffer, buffer, this->m_Length);
}

BufferDataStream::BufferDataStream(uint64_t length) : m_Length(length) {
    this->m_Buffer = new uint8_t[length];
}

BufferDataStream::~BufferDataStream() {
    delete this->m_Buffer;
    m_ReadIndex = m_WriteIndex = m_Length = 0;
}

uint64_t BufferDataStream::GetReadIndex() {
    return this->m_ReadIndex;
}

uint64_t BufferDataStream::GetWriteIndex() {
    return this->m_WriteIndex;
}

void BufferDataStream::SeekRead(uint64_t pos) {
    this->m_ReadIndex = pos;
}

void BufferDataStream::SkipRead(int64_t offset) {
    this->m_ReadIndex += offset;
}

void BufferDataStream::SeekWrite(uint64_t pos) {
    this->m_WriteIndex = pos;
}

void BufferDataStream::SkipWrite(int64_t offset) {
    this->m_WriteIndex += offset;
}

bool BufferDataStream::EndOfStream() const {
    return this->m_ReadIndex >= this->m_Length;
}

size_t BufferDataStream::Read(void* out, size_t size) {
    size = std::min(size, this->m_Length - this->m_ReadIndex);
    std::memcpy(out, this->m_Buffer + this->m_ReadIndex, size);
    this->m_ReadIndex += size;
    return size;
}

size_t BufferDataStream::Write(const void* in, size_t size) {
    size = std::min(size, this->m_Length - this->m_WriteIndex);
    std::memcpy(this->m_Buffer + this->m_WriteIndex, in, size);
    this->m_WriteIndex += size;
    return size;
}

void BufferDataStream::PeekBytes(void* out, uint64_t pos, size_t size) {
    pos = std::clamp(pos, uint64_t(0), this->m_Length);
    size = std::min(size, this->m_Length - pos);
    std::memcpy(out, this->m_Buffer + pos, size);
}

uint64_t BufferDataStream::GetLength() {
    return this->m_Length;
}

void BufferDataStream::WriteToFile(uint64_t pos, uint64_t size, const std::filesystem::path &path, uint64_t chunkSize) {
    pos = std::clamp(pos, uint64_t(0), m_Length);
    size = std::min(size, m_Length - pos);
    uint64_t *chunk = new uint64_t[chunkSize];
    uint64_t end = 0;
    for (uint64_t i = 0; i < size; i += chunkSize) {
        end = std::min(i + chunkSize, size);
        PeekBytes(chunk, pos + i, end - i);
        std::ofstream packfile;
        packfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        packfile.open(path, std::ios::binary | std::ios::app);
        packfile.write(reinterpret_cast<const char*>(chunk), static_cast<std::streamsize>(end - i));
        packfile.close();
    }
    delete[] chunk;
}

void BufferDataStream::WriteToFile(uint64_t size, const std::filesystem::path &path, uint64_t chunkSize) {
    BufferDataStream::WriteToFile(0, size, path, chunkSize);
}

void BufferDataStream::WriteToFile(const std::filesystem::path &path, uint64_t chunkSize) {
    BufferDataStream::WriteToFile(0, m_Length, path, chunkSize);
}

}