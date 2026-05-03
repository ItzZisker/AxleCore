#pragma once

#include "axle/graphics/image/AX_ImageLoader.hpp"

#include "axle/data/AX_IDataStream.hpp"
#include "axle/data/AX_DataStreamImplBuffer.hpp"
#include "axle/data/AX_DataEndianness.hpp"
#include "axle/data/AX_DataTemplates.hpp"

#include "axle/utils/AX_MagicPool.hpp"
#include "axle/utils/AX_Coordination.hpp"
#include "axle/utils/AX_Types.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <array>

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
    Vertex,
    Index,
    SkinWeights,
    MorphTargetPosition,
    MorphTargetNormal
};

enum class VertexFormat {
    Uv1, Uv1WTangents,
    Uv2, Uv2WTangents,
    Uv4, Uv4WTangents,
    Uv8, Uv8WTangents,
};

struct VertexFormatDesc {
    uint32_t stride;
    uint32_t uvCount;
    bool hasTangents;

    // typed cast helper
    void* (*Cast)(void* raw);
};

template <int UVCount, bool HasTangents>
struct AssetVertex_T {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords[UVCount];

    std::conditional_t<HasTangents, glm::vec3, std::monostate> tangent;
    std::conditional_t<HasTangents, glm::vec3, std::monostate> biTangent;
};

struct AssetVertexUv1       : public AssetVertex_T<1, false> {};
struct AssetVertexUv1WTan   : public AssetVertex_T<1, true>  {};
struct AssetVertexUv2       : public AssetVertex_T<2, false> {};
struct AssetVertexUv2WTan   : public AssetVertex_T<2, true>  {};
struct AssetVertexUv4       : public AssetVertex_T<4, false> {};
struct AssetVertexUv4WTan   : public AssetVertex_T<4, true>  {};
struct AssetVertexUv8       : public AssetVertex_T<8, false> {};
struct AssetVertexUv8WTan   : public AssetVertex_T<8, true>  {};

inline void* CastUv1(void* ptr)        { return reinterpret_cast<AssetVertexUv1*>(ptr); }
inline void* CastUv1WTan(void* ptr)    { return reinterpret_cast<AssetVertexUv1WTan*>(ptr); }
inline void* CastUv2(void* ptr)        { return reinterpret_cast<AssetVertexUv2*>(ptr); }
inline void* CastUv2WTan(void* ptr)    { return reinterpret_cast<AssetVertexUv2WTan*>(ptr); }
inline void* CastUv4(void* ptr)        { return reinterpret_cast<AssetVertexUv4*>(ptr); }
inline void* CastUv4WTan(void* ptr)    { return reinterpret_cast<AssetVertexUv4WTan*>(ptr); }
inline void* CastUv8(void* ptr)        { return reinterpret_cast<AssetVertexUv8*>(ptr); }
inline void* CastUv8WTan(void* ptr)    { return reinterpret_cast<AssetVertexUv8WTan*>(ptr); }

static const VertexFormatDesc VERTEX_FORMAT_LOOKUP[] = {
    { sizeof(AssetVertexUv1),       1, false, &CastUv1       },
    { sizeof(AssetVertexUv1WTan),   1, true,  &CastUv1WTan   },
    { sizeof(AssetVertexUv2),       2, false, &CastUv2       },
    { sizeof(AssetVertexUv2WTan),   2, true,  &CastUv2WTan   },
    { sizeof(AssetVertexUv4),       4, false, &CastUv4       },
    { sizeof(AssetVertexUv4WTan),   4, true,  &CastUv4WTan   },
    { sizeof(AssetVertexUv8),       8, false, &CastUv8       },
    { sizeof(AssetVertexUv8WTan),   8, true,  &CastUv8WTan   },
};

inline const VertexFormat GetVertexFormat(uint32_t uvCount, bool hasTangents) {
    switch (uvCount) {
        case 1: return hasTangents ? VertexFormat::Uv1WTangents : VertexFormat::Uv1;
        case 2: return hasTangents ? VertexFormat::Uv2WTangents : VertexFormat::Uv2;
        case 3:
        case 4: return hasTangents ? VertexFormat::Uv4WTangents : VertexFormat::Uv4;
        case 5:
        case 6:
        case 7:
        case 8: return hasTangents ? VertexFormat::Uv8WTangents : VertexFormat::Uv8;
    }
    return VertexFormat::Uv1;
}

inline const VertexFormatDesc& GetVertexFormatDesc(VertexFormat fmt) {
    return VERTEX_FORMAT_LOOKUP[static_cast<int>(fmt)];
}

enum class MetaType : uint32_t {
    Float    = 0x1,
    Double   = 0x2,
    String   = 0x3,
    Integer  = 0x4,
    Buffer   = 0x5 // AKA Raw blob
};

class Metadata {
public:
    using Value = std::variant<float, double, std::string, int, utils::SRaw>;

    Metadata(MetaType type, utils::SRaw raw);

    Value Get() const;
    utils::SRaw& Raw();
private:
    MetaType m_Type;
    utils::SRaw m_Copy;
};

using MetadataMap = std::unordered_map<std::string, Metadata>;

struct AssetBuffer {
    AssetBufferType type;
    uint32_t stride{0};
    uint32_t count{0};
    MetadataMap metadata;
    utils::URaw raw;   // raw byte buffer (unowned or cow'd)

    inline data::BufferDataStream Stream() {
        return data::BufferDataStream(utils::URawView(raw.data(), raw.size()));
    }
};

struct AssetTexture {
    uint32_t id;
    std::string path;
    gfx::Image image; 
};

struct PBRProps {
    float metallicFactor    {1.0f};
    float roughnessFactor   {1.0f};
    float normalScale       {1.0f};
    float occlusionStrength {1.0f};
    glm::vec4 emissiveColor = glm::vec4(1.0f);
    float transparencyFactor = 0.0f;
    float alphaTest = 0.0f;
};

struct MaterialProps {
    glm::vec3 ior{1.0f};     // index of refraction
    float shininess{32.0f};
    float minOpacity{0.7f};
    float maxOpacity{1.0f};
    float opacity{1.0f};
    float F0{0.04f};

    bool isTransparent{false};
    bool hasDisplacement{false};
    bool hasRoughness{true};

    glm::vec4 baseColor{1.0f};
    glm::vec4 diffuseColor{1.0f};
    glm::vec4 specularColor{1.0f};

    PBRProps pbr{};
};

enum class MaterialTextureType : int32_t {
    Albedo,
    Specular,
    NormalMap,
    HeightMap,
    Roughness,
    Metallic,
    Emissive,
    AmbientOcclusion,
    Displacement,
    __Last__
};

struct AssetMaterial {
    bool imported{false};
    std::string name;
    MaterialProps props;
    MetadataMap metadata;
    std::array<std::vector<int32_t>, (int32_t)MaterialTextureType::__Last__> textures_idx;
    std::array<uint32_t, (int32_t)MaterialTextureType::__Last__> textures_count;

    AssetMaterial() { textures_count.fill(0); }
};

struct SubMesh {
    uint32_t indexOffset{0};
    uint32_t indexCount{0};
    uint32_t materialId{0};
};

struct AssetMesh {
    uint32_t vertexBufferIdx;
    uint32_t indexBufferIdx;
    uint32_t materialIdx;

    utils::CowSpan<SubMesh> parts;   // submeshes referencing materials
};

enum class AssetShaderType {
    Slang,

    // GLSL
    GLSL_Core_330,
    GLSL_Core_450,
    GLSL_Core_460,

    // GLES
    GLES_2_0,
    GLES_3_0,
    GLES_3_2,

    // Binary representations
    SPIRV_Binary,
    DXBC_Binary,
    DXIL_Binary,

    Unknown
};

struct AssetShader {
    std::string name;
    AssetShaderType type;

    // Typically: sections = concatenated shader stages or raw bytecode
    utils::URaw sections;
};

struct AssetSkeletonTag {};
struct AssetSkeletonHandle : public utils::MagicHandleTagged<AssetSkeletonTag> {};

struct AssetSkeleton {
    struct Joint {
        uint32_t parent{0};
        glm::mat4 inverseBindMatrix{1.0f};
        std::string name;
    };
    std::vector<Joint> joints;
};

struct AnimationClip {
    struct KeyframeVec3 {
        float time{0.0f};
        glm::vec3 value{0.0f};
    };

    struct KeyframeQuat {
        float time{0.0f};
        glm::quat value{1,0,0,0};
    };

    struct Channel {
        uint32_t jointIndex{0};
        std::vector<KeyframeVec3> translation;
        std::vector<KeyframeQuat> rotation;
        std::vector<KeyframeVec3> scale;
    };

    utils::CowSpan<Channel> channels;
    float duration{0.0f};
};

struct MorphTarget {
    utils::CowSpan<glm::vec3> positionDelta;
    utils::CowSpan<glm::vec3> normalDelta;
};

struct MeshBounds {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    float radius{0.0f};
};

struct LightAsset {
    enum class Type { Directional, Point, Spot };

    Type type{Type::Directional};
    glm::vec3 color{1.0f};
    float intensity{1.0f};
};

struct CameraAsset {
    float fov{70.0f};
    float nearPlane{0.01f};
    float farPlane{1000.0f};
};

// blend, depth, culling modes (you likely have these in axle::graphics)
enum class AssetBlendMode {
    Opaque,
    AlphaBlend,
    Additive,
    Multiply
};

enum class AssetCullMode {
    None,
    Front,
    Back
};

struct PipelineAsset {
    uint32_t vertexShaderIdx;
    uint32_t fragmentShaderIdx;

    AssetBlendMode blend{AssetBlendMode::Opaque};
    AssetCullMode cull{AssetCullMode::Back};
};

// an interface like IGraphicsBackend, but we want:
// 1 - AssetSTLAssimpFileImporter: takes STL std::filesystem::path and reads there
// 2 - AssetStreamPackedFileImporter: takes partial stream packed files/buffers (BufferDataStream/FileDataStream) and reads there, oh there is also a custom exporter that does that.
// 3 - AssetpDroidPackedImporter: takes Android AAssetManager and const char* as path as arguments to read from. it uses the same format as (2)

struct AssetImportResult {
    utils::CowSpan<Node> nodes;

    utils::CowSpan<AssetMesh> meshes;
    utils::CowSpan<AssetMaterial> materials;
    utils::CowSpan<AssetBuffer> buffers;
    utils::CowSpan<AssetTexture> textures;

    utils::CowSpan<AssetShader> shaders;

    utils::CowSpan<AssetSkeleton> skeletons;
    utils::CowSpan<AnimationClip> animations;

    utils::CowSpan<MorphTarget> morphTargets;

    utils::CowSpan<LightAsset> lights;
    utils::CowSpan<CameraAsset> cameras;

    MetadataMap metadata;
};

enum class AssetImportFlag : uint32_t {
    IncludePBR   = (1 << 0),
    CalcTangents = (1 << 1)
};

struct AssetImportDesc {
    uint32_t flags{uint32_t(AssetImportFlag::CalcTangents)};
    float opaquenessThreshold{0.05f};
};

class IAssetImporter {
protected:
    AssetImportDesc m_Desc;
public:
    IAssetImporter(const AssetImportDesc& desc) : m_Desc(desc) {}
    virtual ~IAssetImporter() = default;

    bool HasFlag(AssetImportFlag flag) const {
        return m_Desc.flags & static_cast<uint32_t>(flag);
    }

    virtual utils::ExResult<AssetImportResult> Import() = 0;
    virtual std::string GetImporterName() const = 0;
};


} // namespace axle::assets
