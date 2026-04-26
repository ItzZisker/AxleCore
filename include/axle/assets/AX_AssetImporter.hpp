#pragma once

#include "axle/graphics/base/image/AX_Image.hpp"

#include "axle/data/AX_IDataStream.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_DataEndianness.hpp"
#include "axle/data/AX_DataTemplates.hpp"

#include "axle/utils/AX_Coordination.hpp"
#include "axle/utils/AX_Types.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace axle::assets
{

struct Node {
    int32_t nodeId{-1};
    int32_t meshId{-1};

    std::string name{"ROOT"};
    utils::Coordination transform{};
    
    std::vector<Node> children{};

    SharedPtr<void> misc{nullptr};
};

enum class AssetBufferType {
    MeshTrigVertices,
    MeshTrigVerticesExt,
    MeshTrigIndices,
    MeshTexture,
    PipelineShader
};

template <typename Ret_Type>
class IAssetBuffer {
protected:
    AssetBufferType m_Type;
    SharedPtr<data::IDataStream> m_Data;

    SharedPtr<void> m_Misc{nullptr};
public:
    explicit IAssetBuffer(SharedPtr<data::IDataStream> data, SharedPtr<void> misc = {nullptr})
        : m_Data(data), m_Misc(misc) {}
    ~IAssetBuffer() = default;

    SharedPtr<void> GetTag() const { return m_Misc; }
    void SetTag(SharedPtr<void> misc) { m_Misc = misc; }

    const AssetBufferType& GetType() const { return m_Type; }
    SharedPtr<data::IDataStream> GetRawData() const { return m_Data; }

    virtual uint64_t GetRetLength() = 0;

    virtual utils::ExResult<uint64_t> GetReadIndex() = 0;
    virtual utils::ExError SetReadIndex(uint64_t element_index) = 0;

    virtual utils::ExResult<Ret_Type> ReadAt(uint64_t element_index) = 0;
    virtual utils::ExError ReadNext(Ret_Type& out) = 0;
};

struct AssetVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 biTangent;
};

class AssetBufferVertex : public IAssetBuffer<AssetVertex> {
private:
    const uint32_t m_MeshId{0};

    static constexpr uint64_t VERTEX_SIZE = sizeof(AssetVertex);
public:
    explicit AssetBufferVertex(SharedPtr<data::IDataStream> data,
                      uint32_t meshId,
                      SharedPtr<void> misc = {nullptr})
        : m_MeshId(meshId), IAssetBuffer(data, misc) {
            data->SeekRead(0);
            data->SeekWrite(0);
    }

    uint32_t GetMeshId() const {
        return m_MeshId;
    }

    uint64_t GetRetLength() override {
        return VERTEX_SIZE;
    }

    utils::ExResult<uint64_t> GetReadIndex() override {
        return m_Data->GetReadIndex() / VERTEX_SIZE;
    }

    utils::ExError SetReadIndex(uint64_t index) override {
        return m_Data->SeekRead(index * VERTEX_SIZE);
    }

    utils::ExResult<AssetVertex> ReadAt(uint64_t index) override {
        auto prev = m_Data->GetReadIndex();

        AX_PROPAGATE_ERROR(m_Data->SeekRead(index * VERTEX_SIZE));

        auto& stream = *m_Data;
        AssetVertex res;

        AX_SET_OR_PROPAGATE(res.position,   data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(res.normal,     data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(res.uv,         data::LE_ReadVec2(stream));
        AX_SET_OR_PROPAGATE(res.tangent,    data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(res.biTangent,  data::LE_ReadVec3(stream));

        m_Data->SeekRead(prev);
        return res;
    }

    utils::ExError ReadNext(AssetVertex& out) override {
        auto prev = m_Data->GetReadIndex();
        auto& stream = *m_Data;

        AX_PROPAGATE_ERROR(m_Data->SeekRead(prev + VERTEX_SIZE));
        AX_SET_OR_PROPAGATE(out.position,   data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(out.normal,     data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(out.uv,         data::LE_ReadVec2(stream));
        AX_SET_OR_PROPAGATE(out.tangent,    data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(out.biTangent,  data::LE_ReadVec3(stream));

        return utils::ExError::NoError();
    }
};

struct AssetTrigIndex {
    uint32_t value;
};

class AssetBufferIndex : public IAssetBuffer<AssetTrigIndex> {
private:
    const uint32_t m_MeshId{0};

    static constexpr uint64_t INDEX_SIZE = sizeof(AssetTrigIndex);
public:
    explicit AssetBufferIndex(SharedPtr<data::IDataStream> data,
                      uint32_t meshId,
                      SharedPtr<void> misc = {nullptr})
        : m_MeshId(meshId), IAssetBuffer(data, misc) {}

    uint32_t GetMeshId() const {
        return m_MeshId;
    }

    uint64_t GetRetLength() override {
        return INDEX_SIZE;
    }

    utils::ExResult<uint64_t> GetReadIndex() override {
        return m_Data->GetReadIndex() / INDEX_SIZE;
    }

    utils::ExError SetReadIndex(uint64_t index) override {
        return m_Data->SeekRead(index * INDEX_SIZE);
    }

    utils::ExResult<AssetTrigIndex> ReadAt(uint64_t index) override {
        auto prev = m_Data->GetReadIndex();

        AX_PROPAGATE_ERROR(m_Data->SeekRead(index * INDEX_SIZE));

        auto& stream = *m_Data;
        AssetTrigIndex res;

        auto err = data::LE_Read<uint32_t>(stream, &res.value);
        if (!err.has_value()) return err.error();

        m_Data->SeekRead(prev);
        return res;
    }

    utils::ExError ReadNext(AssetTrigIndex& out) override {
        auto prev = m_Data->GetReadIndex();
        auto& stream = *m_Data;

        AX_PROPAGATE_ERROR(m_Data->SeekRead(prev + INDEX_SIZE));
        auto err = data::LE_Read<uint32_t>(stream, &out.value);
        if (!err.has_value()) return err.error();

        return utils::ExError::NoError();
    }
};

union AssetTexturePixel {
    struct { uint8_t r, g, b, a; }  v8;
    struct { uint16_t r, g, b, a; } v16;
    struct { float r, g, b, a; }    vf;
};

class AssetBufferTexture : public IAssetBuffer<AssetTexturePixel> {
private:
    const uint32_t m_MeshId{0};

    uint32_t m_Width, m_Height;
    gfx::ImageFormat m_Format{gfx::ImageFormat::Unknown};
    gfx::ImageFormatInfo m_FormatInfo{};

    static constexpr uint64_t ASSET_TEXEL_SIZE = sizeof(AssetTexturePixel);
public:
    AssetBufferTexture(gfx::Image image,
                        uint32_t meshId,
                        SharedPtr<void> misc = {nullptr})
        : m_MeshId(meshId), m_Format(image.format),
            IAssetBuffer(std::make_shared<data::BufferDataStream>(image.bytes), misc)
    {
        m_FormatInfo = gfx::Img_GetFormatInfo(image.format);
        m_Width = image.width;
        m_Height = image.height;
    }

    AssetBufferTexture(SharedPtr<data::IDataStream> data,
                        gfx::ImageFormat format,
                        uint32_t meshId,
                        SharedPtr<void> misc = {nullptr})
        : m_MeshId(meshId), m_Format(format), IAssetBuffer(data, misc)
    {
        m_FormatInfo = gfx::Img_GetFormatInfo(format);
    }

    uint32_t GetMeshId() const {
        return m_MeshId;
    }

    uint32_t GetWidth() const { 
        return m_Width;
    }

    uint32_t GetHeight() const { 
        return m_Height;
    }

    gfx::ImageFormat GetImageFormat() const { 
        return m_Format;
    }

    uint64_t GetRetLength() override {
        switch (m_Format) {
            case gfx::ImageFormat::R8:          return 1;
            case gfx::ImageFormat::RG8:         return 2;
            case gfx::ImageFormat::RGB8:        return 3;
            case gfx::ImageFormat::RGBA8:       return 4;
            case gfx::ImageFormat::R16:         return 2;
            case gfx::ImageFormat::RG16:        return 4;
            case gfx::ImageFormat::RGB16:       return 6;
            case gfx::ImageFormat::RGBA16:      return 8;
            case gfx::ImageFormat::R32F:        return 4;
            case gfx::ImageFormat::RG32F:       return 8;
            case gfx::ImageFormat::RGB32F:      return 12;
            case gfx::ImageFormat::RGBA32F:     return 16;
            case gfx::ImageFormat::Unknown:     return 0;
        }
    }

    utils::ExResult<uint64_t> GetReadIndex() override {
        return m_Data->GetReadIndex() / GetRetLength();
    }

    utils::ExError SetReadIndex(uint64_t index) override {
        return m_Data->SeekRead(index * GetRetLength());
    }

    utils::ExResult<AssetTexturePixel> ReadRaw(data::BufferDataStream tempBuffer) {
        AssetTexturePixel res;
        switch (m_Format) {
            case gfx::ImageFormat::R8:
                AX_PROPAGATE_RESULT_ERROR(tempBuffer.Read(&res.v8.r, 1));
                return res;
            case gfx::ImageFormat::RG8:
                AX_PROPAGATE_RESULT_ERROR(tempBuffer.Read(&res.v8.r, 2));
                return res;
            case gfx::ImageFormat::RGB8:
                AX_PROPAGATE_RESULT_ERROR(tempBuffer.Read(&res.v8.r, 3));
                return res;
            case gfx::ImageFormat::RGBA8:
                AX_PROPAGATE_RESULT_ERROR(tempBuffer.Read(&res.v8.r, 4));
                return res;
            case gfx::ImageFormat::R16:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.r));
                return res;
            case gfx::ImageFormat::RG16:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.r));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.g));
                return res;
            case gfx::ImageFormat::RGB16:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.r));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.g));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.b));
                return res;
            case gfx::ImageFormat::RGBA16:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.r));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.g));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.b));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<uint16_t>(tempBuffer, &res.v16.a));
                return res;
            case gfx::ImageFormat::R32F:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.r));
                return res;
            case gfx::ImageFormat::RG32F:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.r));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.g));
                return res;
            case gfx::ImageFormat::RGB32F:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.r));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.g));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.b));
                return res;
            case gfx::ImageFormat::RGBA32F:
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.r));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.g));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.b));
                AX_PROPAGATE_RESULT_ERROR(data::LE_Read<float>(tempBuffer, &res.vf.a));
                return res;
            case gfx::ImageFormat::Unknown:
                return utils::ExError("Unknown Image type!");
        }
        return res;
    }

    utils::ExResult<AssetTexturePixel> ReadAt(uint64_t index) override {
        auto prev = m_Data->GetReadIndex();
        auto retL = GetRetLength();
        AX_PROPAGATE_ERROR(m_Data->SeekRead(index * retL));

        uint8_t raw[ASSET_TEXEL_SIZE];
        AX_PROPAGATE_RESULT_ERROR(m_Data->Read(raw, retL));
    
        data::BufferDataStream stream(utils::Span(raw, retL));
        stream.Open();
        AX_PROPAGATE_RESULT(ReadRaw(stream));
    }

    utils::ExError ReadNext(AssetTexturePixel& out) override {
        auto prev = m_Data->GetReadIndex();
        auto retL = GetRetLength();
        AX_PROPAGATE_ERROR(m_Data->SeekRead(prev + retL));

        uint8_t raw[ASSET_TEXEL_SIZE];
        AX_PROPAGATE_RESULT_ERROR(m_Data->Read(raw, retL));
    
        data::BufferDataStream stream(utils::Span(raw, retL));
        stream.Open();

        auto res = ReadRaw(stream);
        if (res.has_value()) {
            out = res.value();
        } else {
            return res.error();
        }
    }
};

class AssetVertexExt {
private:
    data::BufferDataStream m_Stream;
public:
    explicit AssetVertexExt(uint64_t length) : m_Stream(length) {
        m_Stream.Open().ThrowIfValid();
    }

    data::BufferDataStream& GetStream() {
        return m_Stream;
    }
};

class AssetBufferVertexExt : public IAssetBuffer<AssetVertexExt> {
private:
    const uint32_t m_MeshId{0};
    const uint64_t m_Size{0};
public:
    explicit AssetBufferVertexExt(SharedPtr<data::IDataStream> data,
                    uint64_t retSize,
                    uint32_t meshId,
                    SharedPtr<void> misc = {nullptr})
        : m_MeshId(meshId), m_Size(retSize), IAssetBuffer(data, misc) {
            data->SeekRead(0);
            data->SeekWrite(0);
    }

    uint32_t GetMeshId() const {
        return m_MeshId;
    }

    uint64_t GetRetLength() override {
        return m_Size;
    }

    utils::ExResult<uint64_t> GetReadIndex() override {
        return m_Data->GetReadIndex() / GetRetLength();
    }

    utils::ExError SetReadIndex(uint64_t index) override {
        return m_Data->SeekRead(index * GetRetLength());
    }

    utils::ExResult<AssetVertexExt> ReadAt(uint64_t index) override {
        auto prev = m_Data->GetReadIndex();

        AX_PROPAGATE_ERROR(m_Data->SeekRead(index * GetRetLength()));

        auto& stream = *m_Data;
        AssetVertexExt res(GetRetLength());
        auto& res_stream = res.GetStream();

        
        stream.Read();

        AX_SET_OR_PROPAGATE(res.position,   data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(res.normal,     data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(res.uv,         data::LE_ReadVec2(stream));
        AX_SET_OR_PROPAGATE(res.tangent,    data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(res.biTangent,  data::LE_ReadVec3(stream));

        m_Data->SeekRead(prev);
        return res;
    }

    utils::ExError ReadNext(AssetVertex& out) override {
        auto prev = m_Data->GetReadIndex();
        auto& stream = *m_Data;

        AX_PROPAGATE_ERROR(m_Data->SeekRead(prev + VERTEX_SIZE));
        AX_SET_OR_PROPAGATE(out.position,   data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(out.normal,     data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(out.uv,         data::LE_ReadVec2(stream));
        AX_SET_OR_PROPAGATE(out.tangent,    data::LE_ReadVec3(stream));
        AX_SET_OR_PROPAGATE(out.biTangent,  data::LE_ReadVec3(stream));

        return utils::ExError::NoError();
    }
};


}