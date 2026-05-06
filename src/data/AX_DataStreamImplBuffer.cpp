#include "axle/data/AX_DataStreamImplBuffer.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

namespace axle::data
{

BufferDataStream::BufferDataStream(utils::URawView bufferView)
    : m_BufferView(bufferView) {}

BufferDataStream::BufferDataStream(uint64_t length)
    : m_BufferView(nullptr, length) {
}

BufferDataStream::~BufferDataStream() {
    m_ReadIndex = m_WriteIndex = 0;
}

utils::ExError BufferDataStream::Open() {
    if (m_BufferView.handle() == nullptr) {
        m_BufferHeld = std::vector<uint8_t>((std::size_t) m_BufferView.size(), 0);
        m_BufferView = {m_BufferHeld.data(), m_BufferHeld.size()};
    }
    m_Opened = true;
    return utils::ExError::NoError();
}

uint64_t BufferDataStream::GetReadIndex() {
    if (!m_Opened) return UINT64_MAX;
    return m_ReadIndex;
}

uint64_t BufferDataStream::GetWriteIndex() {
    if (!m_Opened) return UINT64_MAX;
    return m_WriteIndex;
}

uint64_t BufferDataStream::GetLength() {
    if (!m_Opened) return 0;
    return m_BufferView.size();
}

utils::ExError BufferDataStream::SeekRead(uint64_t pos) {
    if (!m_Opened) return {"Stream is not open"};
    m_ReadIndex = pos;
    return utils::ExError::NoError();
}

utils::ExError BufferDataStream::SkipRead(int64_t offset) {
    if (!m_Opened) return {"Stream is not open"};
    m_ReadIndex += offset;
    return utils::ExError::NoError();
}

utils::ExError BufferDataStream::SeekWrite(uint64_t pos) {
    if (!m_Opened) return {"Stream is not open"};
    m_WriteIndex = pos;
    return utils::ExError::NoError();
}

utils::ExError BufferDataStream::SkipWrite(int64_t offset) {
    if (!m_Opened) return {"Stream is not open"};
    m_WriteIndex += offset;
    return utils::ExError::NoError();
}

bool BufferDataStream::EndOfStream() const {
    return !m_Opened || m_ReadIndex >= m_BufferView.size();
}

utils::ExResult<std::size_t> BufferDataStream::Read(void* out, std::size_t size) {
    if (!m_Opened) return utils::ExError{"Stream is not open"};
    if (EndOfStream()) return utils::ExError{-5, "Unexpected EOF"};

    size = std::clamp(size, (std::size_t) 0, m_BufferView.size() - m_ReadIndex);
    if (size == 0) return utils::ExError{-5, "Unexpected EOF"};
    
    std::memcpy(out, m_BufferView.handle() + m_ReadIndex, size);
    m_ReadIndex += size;
    
    return size;
}

utils::ExResult<std::size_t> BufferDataStream::Write(const void* in, std::size_t size) {
    if (!m_Opened) return utils::ExError{"Stream is not open"};
    if (EndOfStream()) return utils::ExError(-5, "Unexpected EOF");
    
    size = std::clamp(size, (std::size_t) 0, m_BufferView.size() - m_WriteIndex);
    if (size == 0) return utils::ExError{-5, "Unexpected EOF"};

    if (m_BufferHeld.data() == m_BufferView.handle() &&
        m_WriteIndex + size > m_BufferHeld.size()) {
        m_BufferHeld.resize(m_WriteIndex + size);
        m_BufferView = {m_BufferHeld.data(), m_BufferHeld.size()};
    }

    std::memcpy(m_BufferView.handle() + m_WriteIndex, in, size);
    
    m_WriteIndex += size;
    return size;
}

utils::ExResult<std::size_t> BufferDataStream::Write(uint8_t byte, std::size_t repeat) {
    if (!m_Opened) return utils::ExError{"Stream is not open"};
    if (EndOfStream()) return utils::ExError("Unexpected EOF");
    if (repeat == 0) return utils::ExError::NoError();

    std::vector<uint8_t> bytes(repeat, byte);
    return Write(bytes.data(), repeat);
}

utils::ExError BufferDataStream::PeekBytes(void* out, uint64_t pos, std::size_t size) {
    if (!m_Opened) return {"Stream is not open"};
    if (EndOfStream()) return utils::ExError{"Unexpected EOF"};
    
    pos = std::clamp(pos, uint64_t(0), m_BufferView.size());
    size = std::clamp(size, (std::size_t) 0, m_BufferView.size() - pos);

    if (size == 0) return utils::ExError{"Unexpected EOF"};
    
    std::memcpy(out, m_BufferView.handle() + pos, size);
    return utils::ExError::NoError();
}

utils::ExError BufferDataStream::WriteToFile(uint64_t pos, uint64_t size, const std::filesystem::path &path, uint64_t chunkSize) {
    if (!m_Opened) return {"Stream is not open"};
    
    pos = std::clamp(pos, uint64_t(0), m_BufferView.size());
    size = std::clamp(size, (std::size_t) 0, m_BufferView.size() - pos);

    if (size == 0) return {"Size too big! size=" + std::to_string(size)};
    
    std::vector<uint64_t> chunk((std::size_t)chunkSize);
    uint64_t end = 0;
    
    for (uint64_t i = 0; i < size; i += chunkSize) {
        end = std::min(i + chunkSize, size);
        PeekBytes(chunk.data(), pos + i, end - i);
        std::ofstream packfile;
        packfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        packfile.open(path, std::ios::binary | std::ios::app);
        packfile.write(reinterpret_cast<const char*>(chunk.data()), static_cast<std::streamsize>(end - i));
        packfile.close();
    }
    return utils::ExError::NoError();
}

utils::ExError BufferDataStream::WriteToFile(uint64_t size, const std::filesystem::path &path, uint64_t chunkSize) {
    return BufferDataStream::WriteToFile(0, size, path, chunkSize);
}

utils::ExError BufferDataStream::WriteToFile(const std::filesystem::path &path, uint64_t chunkSize) {
    return BufferDataStream::WriteToFile(0, m_BufferView.size(), path, chunkSize);
}

}