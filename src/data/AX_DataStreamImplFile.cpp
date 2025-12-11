#include "axle/data/AX_DataStreamImplFile.hpp"

namespace axle::data
{

FileDataStream::FileDataStream(const std::filesystem::path& path, bool read, bool write) {
    std::ios::openmode mode = std::ios::binary;
    if (read)  mode |= std::ios::in;
    if (write) mode |= std::ios::out | std::ios::trunc;
    m_File.open(path, mode);
    if (!m_File.is_open()) throw std::runtime_error("Failed to open file: " + path.string());
    m_Readable = read;
    m_Writable = write;
}

FileDataStream::~FileDataStream() {
    m_File.close();
}

uint64_t FileDataStream::GetReadIndex() {
    return m_Readable ? static_cast<uint64_t>(m_File.tellg()) : -1;
}

uint64_t FileDataStream::GetWriteIndex() {
    return m_Writable ? static_cast<uint64_t>(m_File.tellp()) : -1;
}

void FileDataStream::SeekRead(uint64_t pos) {
    if (!m_Readable) throw std::runtime_error("Stream not open for reading");
    m_File.seekg(static_cast<std::streamoff>(pos));
}

void FileDataStream::SeekWrite(uint64_t pos) {
    if (!m_Writable) throw std::runtime_error("Stream not open for writing");
    m_File.seekp(static_cast<std::streamoff>(pos));
}

void FileDataStream::SkipRead(int64_t offset) {
    if (!m_Readable) throw std::runtime_error("Stream not open for reading");
    m_File.seekg(offset, std::ios::cur);
}

void FileDataStream::SkipWrite(int64_t offset) {
    if (!m_Writable) throw std::runtime_error("Stream not open for writing");
    m_File.seekp(offset, std::ios::cur);
}

bool FileDataStream::EndOfStream() const {
    return !m_Readable || m_File.eof();
}

size_t FileDataStream::Read(void* out, size_t size) {
    if (!m_Readable) throw std::runtime_error("Stream not open for reading");
    m_File.read(reinterpret_cast<char*>(out), static_cast<std::streamsize>(size));
    return static_cast<size_t>(m_File.gcount());
}

size_t FileDataStream::Write(const void* in, size_t size) {
    if (!m_Writable) throw std::runtime_error("Stream not open for writing");
    m_File.write(reinterpret_cast<const char*>(in), static_cast<std::streamsize>(size));
    return size;
}

uint64_t FileDataStream::GetLength() {
    if (m_CachedLength) return m_CachedLength;
    std::streampos current = m_File.tellg();
    m_File.seekg(0, std::ios::end);
    std::streampos end = m_File.tellg();
    m_File.seekg(current); // restore previous position
    return (this->m_CachedLength = static_cast<uint64_t>(end));
}

}