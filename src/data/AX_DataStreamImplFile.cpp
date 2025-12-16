#include "axle/data/AX_DataStreamImplFile.hpp"
#include <iostream>
#include <ostream>

namespace axle::data
{

FileDataStream::FileDataStream(const std::filesystem::path& path, bool read, bool write) {
    std::ios::openmode mode = std::ios::binary;
    if (read)  mode |= std::ios::in;
    if (write) mode |= std::ios::out | std::ios::trunc;
    m_File.open(path, mode);
    if (m_File.fail() || !m_File.is_open()) throw std::runtime_error("Failed to open file: " + path.string());
    m_Readable = read;
    m_Writable = write;
}

FileDataStream::~FileDataStream() {
    m_File.close();
}

uint64_t FileDataStream::GetReadIndex() {
    if (!m_Readable) throw std::runtime_error("Stream not open for reading");

    m_File.clear();
    auto pos = m_File.tellg();

    if (pos == std::streampos(-1))
        throw std::runtime_error("tellg failed");

    return static_cast<uint64_t>(pos);
}

uint64_t FileDataStream::GetWriteIndex() {
    if (!m_Writable) throw std::runtime_error("Stream not open for writing");
    return static_cast<uint64_t>(m_File.tellp());
}

void FileDataStream::SeekRead(uint64_t pos) {
    if (!m_Readable) throw std::runtime_error("Stream not open for reading");

    m_File.clear();
    m_File.seekg(static_cast<std::streamoff>(pos));

    if (m_File.fail())
        throw std::runtime_error("SeekRead failed");
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

std::size_t FileDataStream::Read(void* out, std::size_t size) {
    if (!m_Readable) throw std::runtime_error("Stream not open for reading");
    m_File.read(reinterpret_cast<char*>(out), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(m_File.gcount());
}

std::size_t FileDataStream::Write(const void* in, std::size_t size) {
    if (!m_Writable) throw std::runtime_error("Stream not open for writing");
    m_File.write(reinterpret_cast<const char*>(in), static_cast<std::streamsize>(size));
    return size;
}

uint64_t FileDataStream::GetLength() {
    if (m_CachedLength) return m_CachedLength;
    m_File.clear();
    std::streampos current = m_File.tellg();
    m_File.seekg(0, std::ios::end);
    std::streampos end = m_File.tellg();
    if (end == std::streampos(-1)) throw std::runtime_error("GetLength failed");
    m_File.seekg(current); // restore previous position
    return (this->m_CachedLength = static_cast<uint64_t>(end));
}

}