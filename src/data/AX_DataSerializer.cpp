#include "axle/data/AX_DataSerializer.hpp"

namespace axle::data
{

DataSerializer::DataSerializer(std::shared_ptr<DataStream> stream, uint64_t chunkSize)
    : m_Stream(stream), m_ChunkSize(chunkSize) {}

uint64_t DataSerializer::GetLength() {
    return m_Stream->GetLength();
}

uint64_t DataSerializer::GetWritePos() {
    return m_Stream->GetWriteIndex();
}

void DataSerializer::Write(const void* bytes, size_t size) {
    m_Stream->Write(bytes, size);
}

void DataSerializer::Skip(uint64_t pos) {
    m_Stream->SkipWrite(pos);
}

void DataSerializer::Rewind(uint64_t pos) {
    m_Stream->SkipWrite(-pos);
}

}