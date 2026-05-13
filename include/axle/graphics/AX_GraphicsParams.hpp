#pragma once

#include "axle/utils/AX_Span.hpp"
#include "axle/utils/AX_MagicPool.hpp"

#include "slang.h"

#include <cstdint>
#include <vector>
#include <array>

#define AX_HEX_RGB(hex) \
    ((float)((((uint32_t)hex) >> 16) & 0xFF) / 255.0f), \
    ((float)((((uint32_t)hex) >> 8)  & 0xFF) / 255.0f), \
    ((float)((((uint32_t)hex))       & 0xFF) / 255.0f), \
    1.0f

#define AX_HEX_RGBA(hex) \
    ((float)((((uint32_t)(hex)) >> 24) & 0xFF) / 255.0f), \
    ((float)((((uint32_t)(hex)) >> 16) & 0xFF) / 255.0f), \
    ((float)((((uint32_t)(hex)) >> 8)  & 0xFF) / 255.0f), \
    ((float)(((uint32_t)(hex))        & 0xFF) / 255.0f)

#define AX_HEX_RGB_A(hex, alpha) \
    ((float)((((uint32_t)hex) >> 16) & 0xFF) / 255.0f), \
    ((float)((((uint32_t)hex) >> 8)  & 0xFF) / 255.0f), \
    ((float)((((uint32_t)hex))       & 0xFF) / 255.0f), \
    (alpha)

namespace axle::gfx {

template<typename Tag>
struct ExternalHandle : public utils::MagicHandleTagged<Tag> {};

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage,
    Indirect,
    Staging
};

enum class BufferAccess {
    Immutable,
    Dynamic,
    Stream
};

struct BufferDesc {
    size_t size;
    BufferUsage usage;
    BufferAccess access;
    bool cpuVisible;
    uint32_t bindSlot{0};
};

struct BufferTag {};

using BufferHandle = ExternalHandle<BufferTag>;

enum class SampleCount {
    Sample1 = 1,
    Sample2 = 2,
    Sample4 = 4,
    Sample8 = 8
};

enum class TextureType {
    Texture2D,
    Texture3D,
    Cubemap,
    Array2D
};

enum class TextureUsage : uint32_t {
    Sampled         = 0,
    RenderTarget    = 1 << 0,
    DepthOnly       = 1 << 1,
    DepthStencil    = 1 << 2,
    TransferSrc     = 1 << 3,
    TransferDst     = 1 << 4
};

enum class TextureFormat {
    // 8-bit normalized
    R8_UNORM,
    R8_SNORM,
    R8_UINT,
    R8_SINT,

    RG8_UNORM,
    RG8_SNORM,
    RG8_UINT,
    RG8_SINT,

    RGB8_UNORM,
    RGB8_SNORM,
    RGB8_UINT,
    RGB8_SINT,

    RGBA8_UNORM,
    RGBA8_SNORM,
    RGBA8_UINT,
    RGBA8_SINT,

    BGRA8_UNORM, // swapchain-friendly
    RGBA8_SRGB,

    // 16-bit formats
    R16_UNORM,
    R16_SNORM,
    R16_UINT,
    R16_SINT,
    R16_FLOAT,

    RG16_UNORM,
    RG16_SNORM,
    RG16_UINT,
    RG16_SINT,
    RG16_FLOAT,

    RGB16_UNORM,
    RGB16_SNORM,
    RGB16_UINT,
    RGB16_SINT,
    RGB16_FLOAT,

    RGBA16_UNORM,
    RGBA16_SNORM,
    RGBA16_UINT,
    RGBA16_SINT,
    RGBA16_FLOAT,

    // 32-bit formats
    R32_UINT,
    R32_SINT,
    R32_FLOAT,

    RG32_UINT,
    RG32_SINT,
    RG32_FLOAT,

    RGB32_FLOAT,
    RGBA32_FLOAT,

    // Packed / special
    RGB10A2_UNORM,
    RG11B10_FLOAT,

    // Depth / Stencil
    S8_UINT,
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D32_FLOAT_S8_UINT,

    // Block compression (BCn / S3TC)
    BC1_UNORM,
    BC1_SRGB,
    BC3_UNORM,
    BC3_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UFLOAT,
    BC6H_SFLOAT,
    BC7_UNORM,
    BC7_SRGB,

    // ASTC (mobile)
    ASTC_4x4_UNORM,
    ASTC_4x4_SRGB,
    ASTC_6x6_UNORM,
    ASTC_6x6_SRGB,
    ASTC_8x8_UNORM,
    ASTC_8x8_SRGB,

    // Undefined
    Unknown
};

inline bool TextureFormatIsS3TC(const TextureFormat& fmt) {
    return TextureFormat::BC1_UNORM <= fmt && fmt <= TextureFormat::BC7_SRGB;
}

inline bool TextureFormatIsASTC(const TextureFormat& fmt) {
    return TextureFormat::ASTC_4x4_UNORM <= fmt && fmt <= TextureFormat::ASTC_8x8_SRGB;
}

inline bool TextureFormatIsDepthOnly(const TextureFormat& fmt) {
    return fmt == TextureFormat::D16_UNORM || fmt == TextureFormat::D32_FLOAT;
}

inline bool TextureFormatIsDepthAndStencil(const TextureFormat& fmt) {
    return fmt == TextureFormat::D24_UNORM_S8_UINT || fmt == TextureFormat::D32_FLOAT_S8_UINT;
}

inline bool TextureFormatIsStencilOnly(const TextureFormat& fmt) {
    return fmt == TextureFormat::S8_UINT;
}

enum class TextureWrap {
    Repeat,             // GL_REPEAT / VK_SAMPLER_ADDRESS_MODE_REPEAT / D3D11_TEXTURE_ADDRESS_WRAP
    MirrorRepeat,       // GL_MIRRORED_REPEAT / VK_MIRROR_REPEAT / D3D11_TEXTURE_ADDRESS_MIRROR
    ClampToEdge,        // GL_CLAMP_TO_EDGE / VK_CLAMP_TO_EDGE / D3D11_TEXTURE_ADDRESS_CLAMP
    ClampToBorder       // GL_CLAMP_TO_BORDER / VK_CLAMP_TO_BORDER / D3D11_TEXTURE_ADDRESS_BORDER
};

enum class TextureFilter {
    Nearest,            // GL_NEAREST / VK_FILTER_NEAREST / D3D11_FILTER_MIN_MAG_MIP_POINT
    Linear,             // GL_LINEAR / VK_FILTER_LINEAR / D3D11_FILTER_MIN_MAG_MIP_LINEAR
};

enum class MipmapFilter {
    None, // No mipmapping, only base level
    Nearest,
    Linear,
};

struct TextureBorderColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};
};

enum class TextureCubemapFace {
    NegativeX,
    NegativeY,
    NegativeZ,
    PositiveX,
    PositiveY,
    PositiveZ
};

struct TextureSubDesc {
    uint32_t mipLevel{0};
    bool generateMips{false};
    float aniso{0.0f}; // anisotropic filtering, 0.0 means disabled

    TextureWrap wrapS{TextureWrap::Repeat};
    TextureWrap wrapT{TextureWrap::Repeat};
    TextureWrap wrapR{TextureWrap::Repeat}; // used for 3D or arrays or cubemap

    MipmapFilter mipFilter{MipmapFilter::None};
    TextureFilter minFilter{TextureFilter::Nearest};
    TextureFilter magFilter{TextureFilter::Nearest};

    TextureBorderColor borderColor{0, 0, 0, 0};

    // Only used when UpdateTexture is called, Updating for cubemaps is "per-face"; unlike 2d-arrays and 3d
    TextureCubemapFace updateThisCubemapFace{TextureCubemapFace::NegativeZ};
};

struct TextureDesc {
    TextureType type;
    uint32_t width{1};
    uint32_t height{1};
    uint32_t depth{1}; // for 3D textures
    uint32_t layers{1}; // for 2D arrays, cube arrays
    SampleCount samples{SampleCount::Sample1}; // samples

    TextureFormat format;
    TextureUsage usage;
    TextureSubDesc subDesc{};

    // Optional texture data
    utils::CowSpan<utils::URaw> pixelsByLayers{}; // pixels data: for tex 2d: One single element, 2d-arrays/3d: One single element but aligned, for cubemap: 6
};

struct TextureTag {};

using TextureHandle = ExternalHandle<TextureTag>;

// NOTE: You cannot have StgCompute linked along with other shaders! they must be separate shader programs!
enum class ShaderStage { 
    Vertex,
    Fragment,
    Compute,
    Geometry,
    Hull,
    Domain
};

enum class PipelineType {
    Graphics, // Classic vertex -> Tessellation/Geometry -> fragment pipline
    Compute   // Compute ONLY
};

inline SlangStage ToSlangStage(ShaderStage stg) {
    switch (stg) {
        case ShaderStage::Vertex:   return SLANG_STAGE_VERTEX;
        case ShaderStage::Fragment: return SLANG_STAGE_FRAGMENT;
        case ShaderStage::Compute:  return SLANG_STAGE_COMPUTE;
        case ShaderStage::Geometry: return SLANG_STAGE_GEOMETRY;
        case ShaderStage::Hull:     return SLANG_STAGE_HULL;
        case ShaderStage::Domain:   return SLANG_STAGE_DOMAIN;
        default: return SLANG_STAGE_NONE;
    }
}

inline PipelineType ToPipelineType(ShaderStage stg) {
    switch (stg) {
        case ShaderStage::Compute: return PipelineType::Compute;
        default: return PipelineType::Graphics;
    }
}

struct ShaderEntryPoint {
    const char* name;
    ShaderStage stage;
};

struct ShaderDesc {
    const char* sourcePath; // .slang file
    
    const char* entryPointVertex;
    const char* entryPointFragment;
    const char* entryPointCompute;

    // Optional compile defines
    utils::CowSpan<const char*> defines{};
    utils::CowSpan<std::pair<const char*, const char*>> variables{};
};

enum class VertexSemantic : uint8_t {
    Position,
    Normal,
    TexCoord,
    Tangent,
    Bitangent,
    Color,
    BoneIndices,
    BoneWeights,
    Custom
};

enum class VertexAttributeClass {
    Int,
    Float,
    Double // Requires GL410
};

enum class VertexAttributeType {
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64, // Requires GL410
    Float16,
    Float32,
    Float64 // Requires GL410
};

struct VertexTypeDesc {
    VertexAttributeClass _class;
    VertexAttributeType type;
};

struct ShaderVertexInput {
    VertexSemantic semantic;
    uint32_t semanticIndex;

    uint32_t location;
    uint32_t componentCount;

    VertexTypeDesc typeDesc;
};

struct ShaderInputState {
    utils::CowSpan<ShaderVertexInput> inputs{};
};

struct ShaderTag {};

using ShaderHandle = ExternalHandle<ShaderTag>;

enum class ResourceKind : uint8_t {
    Buffer,
    Texture
};

struct ResourceHandle {
    ResourceKind kind{ResourceKind::Buffer};

    uint32_t index{0};
    uint32_t generation{0};

    ResourceHandle() {}
    ResourceHandle(BufferHandle h)
        : kind(ResourceKind::Buffer), index(h.index), generation(h.generation) {}
    ResourceHandle(TextureHandle h)
        : kind(ResourceKind::Texture), index(h.index), generation(h.generation) {}

    BufferHandle AsBuffer() {
        BufferHandle res;
        res.index = index;
        res.generation = generation;
        return res;
    }

    TextureHandle AsTexture() {
        TextureHandle res;
        res.index = index;
        res.generation = generation;
        return res;
    }
};

enum class ResourceState {
    Undefined,

    ComputeWrite,
    ComputeRead,

    FragmentRead,
    FragmentWrite,

    VertexRead,
    IndexRead,

    IndirectRead,

    DepthWrite,
    DepthRead,

    RenderTarget,
    Present
};

struct ResourceTransition {
    ResourceHandle resource;
    ResourceState newState;
};

enum class LoadOp {
    Load,       // keep previous contents
    Clear,      // clear at start
    DontCare    // contents undefined
};

enum class StoreOp {
    Store,
    Discard
};

struct PassOps {
    LoadOp load;
    StoreOp store;
};

struct DefaultRenderPassDesc {
    PassOps colorOps;
    PassOps depthStencilOps;
};

using AttachmentFormat = TextureFormat;

struct AttachmentDesc {
    AttachmentFormat format;
    PassOps passOps;
    SampleCount samples{SampleCount::Sample1};
    bool hasResolve{false};
};

struct RenderPassDesc {
    utils::CowSpan<AttachmentDesc> colorAttachments;
    AttachmentDesc depthStencilAttachment;
    bool hasDepth{false};
    bool hasStencil{false};
};

// used if loadOp == Clear
struct RenderPassClear {
    float clearColor[4] = {0.0f,0.0f,0.0f,1.0f};
    float clearDepth = 1.0f;
    uint8_t clearStencil = 0;
};

struct RenderPassTag {};

using RenderPassHandle = ExternalHandle<RenderPassTag>;

struct FramebufferDesc {
    RenderPassHandle renderPass; // guides behavior (load/store, resolve, sampleCount)
    utils::CowSpan<TextureHandle> colorAttachments;

    bool hasDepth{false};
    bool hasStencil{false};
    TextureHandle depthStencilTexture{}; // optional

    uint32_t width;
    uint32_t height;
};

struct FramebufferTag {};

using FramebufferHandle = ExternalHandle<FramebufferTag>;

struct SurfaceInfo {
    uint32_t width{0};
    uint32_t height{0};
    TextureFormat colorFormat{TextureFormat::RGBA8_UINT};
    TextureFormat depthStencilFormat{TextureFormat::D24_UNORM_S8_UINT};
    uint32_t depthBits{0};
    uint32_t stencilBits{0};
    bool vsync{false};
};

struct SwapchainDesc {
    SurfaceInfo surface;
    uint32_t imageCount;
};

struct SwapchainTag {};

using SwapchainHandle = ExternalHandle<SwapchainTag>;

enum class CullMode {
    None,
    Front,
    Back
};

enum class FillMode {
    Solid,
    Wireframe
};

enum class FrontFace {
    Clockwise,
    CounterClockwise
};

enum class PolyMode {
    Dots,
    Lines,
    Triangles
};

struct RasterState {
    CullMode cull{CullMode::Back};
    FillMode fill{FillMode::Solid};
    FrontFace frontFace{FrontFace::CounterClockwise};
    PolyMode polyMode{PolyMode::Triangles};
};

enum class CompareOp {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class StencilOp {
    Keep,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    IncrementAndWrap,
    DecrementAndWrap
};

struct StencilOpState {
    CompareOp compare{CompareOp::Always};
    uint32_t compareMask{0xFFFFFFFF};
    uint32_t writeMask{0xFFFFFFFF};
    uint32_t reference{0};

    // Operations
    StencilOp failOp{StencilOp::Keep};      // stencil fail
    StencilOp depthFailOp{StencilOp::Keep}; // depth fail
    StencilOp passOp{StencilOp::Keep};      // pass
};

struct DepthStencilState {
    bool depthTest{true};
    bool depthWrite{true};
    CompareOp depthCompare{CompareOp::Less};

    bool stencilTest{false}; // optional
    StencilOpState stencilFront{};
    StencilOpState stencilBack{};
};

enum class BlendFactor {
    Zero,
    One,

    SrcColor,
    OneMinusSrcColor,

    DstColor,
    OneMinusDstColor,

    SrcAlpha,
    OneMinusSrcAlpha,

    DstAlpha,
    OneMinusDstAlpha,

    ConstantColor,
    OneMinusConstantColor,

    ConstantAlpha,
    OneMinusConstantAlpha
};

enum class BlendOp {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

struct BlendState {
    bool enabled{false};

    BlendFactor srcColor{BlendFactor::SrcAlpha};
    BlendFactor dstColor{BlendFactor::OneMinusSrcAlpha};
    BlendOp colorOp{BlendOp::Add};

    BlendFactor srcAlpha{BlendFactor::SrcAlpha};
    BlendFactor dstAlpha{BlendFactor::OneMinusSrcAlpha};
    BlendOp alphaOp{BlendOp::Add};
};

struct MeshVertexAttribute {
    VertexSemantic semantic;
    uint32_t semanticIndex;

    uint32_t componentCount;
 
    uint32_t divisor;
    uint32_t offset;
    uint32_t size;

    VertexTypeDesc typeDesc;
    bool normalized;
};

struct MeshVertexLayout {
    uint32_t stride;
    utils::CowSpan<MeshVertexAttribute> attributes;
};

struct RenderPipelineDesc {
    RenderPassHandle renderPass;
    ShaderHandle shader;
    MeshVertexLayout vertexLayout;

    RasterState raster{};
    DepthStencilState depth{};
    BlendState blend{};
};

struct RenderPipelineTag {};

using RenderPipelineHandle = ExternalHandle<RenderPipelineTag>;

struct ComputePipelineDesc {
    ShaderHandle shader;
    const char* computeEntry;
};

struct ComputePipelineTag {};

using ComputePipelineHandle = ExternalHandle<ComputePipelineTag>;

enum class BindingType {
    UniformBuffer,
    StorageBuffer,
    SampledTexture,
    StorageTexture,
    //Sampler
};

struct Binding {
    uint32_t slot{0};
    BindingType type{};
    ResourceHandle resource;

    uint64_t offset{0}; // for buffer bindings
    uint64_t range{0};  // 0 = full size

    ResourceHandle sampler; // optional (for SampledTexture)
};

struct ResourceSetDesc {
    utils::CowSpan<Binding> bindings{};

    uint32_t layoutID{0}; // layout compatibility id
    uint32_t setIndex{0}; // descriptor set index
    uint32_t version{1};  // increments on update
};

struct ResourceSetTag {};

using ResourceSetHandle = ExternalHandle<ResourceSetTag>;

enum class GraphicsCapEnum {
    BindlessTextures,
    ComputeShaders,
    ShaderStorageBuffers,
    IndirectDraw,
    GeometryShaders,
    Tessellation,
    MultiDrawIndirect,
    SparseTextures,
    RayTracing,
    HalfFloatColorBuffer,
    FullFloatColorBuffer,
    LongPointers,
    Anisotropy,
    __Last__
};

struct GraphicsCaps {
    std::array<bool, (int)GraphicsCapEnum::__Last__ + 1> caps{};

    int32_t maxVertexAttribs{0};
    int32_t maxColorAttachments{0};
    int32_t maxArrayTextureLayers{0};
    int32_t maxDrawBuffers{0};
    int32_t maxUBOBindings{0};
    int32_t maxUBOSize{0};
    int32_t maxSSBOBindings{0};
    int32_t maxSSBOSize{0};
    int32_t maxTextureUnits{0};
    int32_t maxCombinedTextureUnits{0};
    uint32_t maxWorkGroupCount[3]{0, 0, 0};
    float maxAniso{0.0f};

    explicit GraphicsCaps() {
        caps.fill(false);
    }
    bool Has(GraphicsCapEnum _enum) {
        return caps[static_cast<int>(_enum)];
    }
};

struct TextureImageDescriptor {
    gfx::TextureType type;
    gfx::TextureFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t depth;
    uint32_t mip;
};

VertexSemantic ParseSemantic(const char* semanticName);
uint32_t CalcImageSize(const TextureImageDescriptor& desc);

}