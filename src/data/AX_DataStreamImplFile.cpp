#include "axle/data/AX_DataStreamImplFile.hpp"

#include <iostream>
#include <ostream>

namespace axle::data
{

FileDataStream::FileDataStream(const std::filesystem::path& path, bool read, bool write)
    : m_Writable(write), m_Readable(read), m_Path(path) {}

FileDataStream::~FileDataStream() {
    m_File.close();
}

utils::ExError FileDataStream::Open() {
    std::ios::openmode mode = std::ios::binary;

    std::error_code sysErr;
    auto status = std::filesystem::status(m_Path, sysErr);

    if (sysErr) {
        return {sysErr.value(), "Failed to get file status for: \"" + m_Path.string() + "\" Error: " + sysErr.message()};
    }
    if (m_Readable && !std::filesystem::exists(status)) {
        return {"File does not exist, cannot enable read mode: " + m_Path.string()};
    }

    auto hasPerms = static_cast<int32_t>(status.permissions());

    if (m_Readable) {
        auto reqPerms = static_cast<int32_t>(
            std::filesystem::perms::owner_read |
            std::filesystem::perms::group_read |
            std::filesystem::perms::others_read
        );
        if (!(hasPerms & reqPerms)) {
            return {"File open requested READ, but file has no read permissions: " + m_Path.string()};
        }
        mode |= std::ios::in;
    }
    if (m_Writable) {
        auto reqPerms = static_cast<int32_t>(
            std::filesystem::perms::owner_write |
            std::filesystem::perms::group_write |
            std::filesystem::perms::others_write
        );
        if (!(hasPerms & reqPerms)) {
            return {"File open requested WRITE, but file has no write permissions: " + m_Path.string()};
        }
        mode |= std::ios::out | std::ios::trunc;
    }

    m_File.open(m_Path, mode);

    if (!m_File.is_open() || m_File.fail()) {
        return {"Failed to open file stream: " + m_Path.string()};
    } else {
        m_Opened = true;
    }
    return utils::ExError::NoError();
}

uint64_t FileDataStream::GetReadIndex() {
    if (!m_Opened || !m_Readable) return UINT64_MAX;
    m_File.clear();
    auto pos = m_File.tellg();
    if (pos == std::streampos(-1)) return UINT64_MAX;
    return static_cast<uint64_t>(pos);
}

uint64_t FileDataStream::GetWriteIndex() {
    if (!m_Opened || !m_Writable) return UINT64_MAX;
    return static_cast<uint64_t>(m_File.tellp());
}

utils::ExError FileDataStream::SeekRead(uint64_t pos) {
    if (!m_Opened) return {"Stream is not opened"};
    if (!m_Readable) return {"Stream not open for reading"};

    m_File.clear();
    m_File.seekg(static_cast<std::streamoff>(pos));

    if (m_File.fail()) return {"SeekRead failed"};

    return utils::ExError::NoError();
}

utils::ExError FileDataStream::SeekWrite(uint64_t pos) {
    if (!m_Opened) return {"Stream is not opened"};
    if (!m_Writable) return {"Stream not open for writing"};
    m_File.seekp(static_cast<std::streamoff>(pos));
    return utils::ExError::NoError();
}

utils::ExError FileDataStream::SkipRead(int64_t offset) {
    if (!m_Opened) return {"Stream is not opened"};
    if (!m_Readable) return {"Stream not open for reading"};
    m_File.seekg(offset, std::ios::cur);
    return utils::ExError::NoError();
}

utils::ExError FileDataStream::SkipWrite(int64_t offset) {
    if (!m_Opened) return {"Stream is not opened"};
    if (!m_Writable) return {"Stream not open for writing"};
    m_File.seekp(offset, std::ios::cur);
    return utils::ExError::NoError();
}

bool FileDataStream::EndOfStream() const {
    return !m_Opened || !m_Readable || m_File.eof();
}

utils::ExResult<std::size_t> FileDataStream::Read(void* out, std::size_t size) {
    if (!m_Opened) return utils::ExError{"Stream is not opened"};
    if (!m_Readable) return utils::ExError{"Stream not open for reading"};
    m_File.read(reinterpret_cast<char*>(out), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(m_File.gcount());
}

utils::ExResult<std::size_t> FileDataStream::Write(const void* in, std::size_t size) {
    if (!m_Opened) return utils::ExError{"Stream is not opened"};
    if (!m_Writable) return utils::ExError{"Stream not open for writing"};
    m_File.write(reinterpret_cast<const char*>(in), static_cast<std::streamsize>(size));
    return size;
}

utils::ExResult<std::size_t> FileDataStream::Write(uint8_t byte, std::size_t repeat) {
    if (!m_Opened) return utils::ExError{"Stream is not opened"};
    if (!m_Writable) return utils::ExError{"Stream not open for writing"};
    std::vector<uint8_t> bytes(repeat, byte);
    m_File.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(repeat));
    return repeat;
}

uint64_t FileDataStream::GetLength() {
    if (!m_Opened) return 0;
    m_File.clear();
    std::streampos current = m_File.tellg();
    m_File.seekg(0, std::ios::end);
    std::streampos end = m_File.tellg();
    if (end == std::streampos(-1)) return 0;
    m_File.seekg(current); // restore previous position
    return static_cast<uint64_t>(end);
}

}