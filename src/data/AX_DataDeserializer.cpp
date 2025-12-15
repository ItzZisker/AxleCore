#include "axle/data/AX_DataDeserializer.hpp"

namespace axle::data
{

DataDeserializer::DataDeserializer(std::shared_ptr<DataStream> stream, uint64_t chunkSize)
    : m_Stream(stream), m_ChunkSize(chunkSize) {}

uint64_t DataDeserializer::GetLength() {
    return m_Stream->GetLength();
}

uint64_t DataDeserializer::GetReadPos() {
    return m_Stream->GetReadIndex();
}

uint8_t DataDeserializer::ReadByte() {
    uint8_t byte;
    m_Stream->Read(&byte, 1);
    return byte;
}

void DataDeserializer::Read(void* out, size_t size) {
    m_Stream->Read(out, size);
}

void DataDeserializer::Rewind(uint64_t offset_inv) {
    m_Stream->SkipRead(-offset_inv);
}

void DataDeserializer::Skip(uint64_t offset) {
    m_Stream->SkipRead(offset);
}

}