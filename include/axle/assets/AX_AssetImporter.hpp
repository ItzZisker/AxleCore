#pragma once

#include "axle/graphics/base/image/AX_Image.hpp"

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
    Texture,
    SkinWeights,
    MorphTargetPosition,
    MorphTargetNormal
};

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
    const MetaType m_Type;
    utils::SRaw m_Copy;
};

using MetadataMap = std::unordered_map<std::string, Metadata>;

struct AssetBufferTag {};
struct AssetBufferHandle : public utils::MagicHandleTagged<AssetBufferTag> {};

struct AssetBuffer : public utils::MagicInternal<AssetBufferHandle> {
    AssetBufferType type;
    uint32_t stride{0};
    uint32_t count{0};
    MetadataMap metadata{};
    utils::URaw raw;   // raw byte buffer (unowned or cow'd)
};

struct AssetMaterialTag {};
struct AssetMaterialHandle : public utils::MagicHandleTagged<AssetMaterialTag> {};

struct PBRProps {
    float metallic    {1.0f};
    float roughness   {1.0f};
    float normal      {1.0f};
    float occlusion   {1.0f};
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
    bool hasDisplacement{true};
    bool hasRoughness{true};

    glm::vec4 baseColor{1.0f};
    glm::vec4 diffuseColor{1.0f};
    glm::vec4 specularColor{1.0f};

    PBRProps pbr{};
};

struct AssetMaterial : public utils::MagicInternal<AssetMaterialHandle> {
    std::string name;
    MaterialProps props;
    MetadataMap metadata;
};

struct SubMesh {
    uint32_t indexOffset{0};
    uint32_t indexCount{0};
    uint32_t materialId{0};
};

struct AssetMesh {
    AssetBufferHandle vertices;
    AssetBufferHandle indices;
    AssetMaterialHandle material;

    utils::CowSpan<SubMesh> parts;   // submeshes referencing materials
};

struct AssetShaderTag {};
struct AssetShaderHandle : public utils::MagicHandleTagged<AssetShaderTag> {};

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
    AssetShaderHandle vertex;
    AssetShaderHandle fragment;

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

    utils::CowSpan<AssetShader> shaders;

    utils::CowSpan<AssetSkeleton> skeletons;
    utils::CowSpan<AnimationClip> animations;

    utils::CowSpan<MorphTarget> morphTargets;

    utils::CowSpan<LightAsset> lights;
    utils::CowSpan<CameraAsset> cameras;

    MetadataMap metadata;
};

class IAssetImporter {
public:
    virtual ~IAssetImporter() = default;

    virtual utils::ExResult<AssetImportResult> Import() = 0;

    virtual std::string GetImporterName() const = 0;
};


} // namespace axle::assets
