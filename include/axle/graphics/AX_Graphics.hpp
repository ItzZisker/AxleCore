#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include "slang.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

#define AX_HEX_RGB(hex) \
    ((float)((hex >> 16) & 0xFF) / 255.0f), \
    ((float)((hex >> 8)  & 0xFF) / 255.0f), \
    ((float)((hex)       & 0xFF) / 255.0f), \
    1.0f

#define AX_HEX_RGBA(hex) \
    ((float)((hex >> 24) & 0xFF) / 255.0f), \
    ((float)((hex >> 16) & 0xFF) / 255.0f), \
    ((float)((hex >> 8)  & 0xFF) / 255.0f), \
    ((float)((hex)       & 0xFF) / 255.0f)

#define AX_HEX_RGB_A(hex, alpha) \
    ((float)((hex >> 16) & 0xFF) / 255.0f), \
    ((float)((hex >> 8)  & 0xFF) / 255.0f), \
    ((float)((hex)       & 0xFF) / 255.0f), \
    (alpha)

namespace axle::gfx {

template<typename Tag>
struct ExternalHandle {
    uint32_t index;
    uint32_t generation;
};

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

struct BufferTag {
    const BufferDesc desc;
};

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

struct TextureBorderColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};
};

struct TextureSubDesc {
    uint32_t mipLevels{0};
    bool generateMips{false};
    float aniso{0.0f}; // anisotropic filtering, 0.0 means disabled

    TextureWrap wrapS{TextureWrap::Repeat};
    TextureWrap wrapT{TextureWrap::Repeat};
    TextureWrap wrapR{TextureWrap::Repeat}; // used for 3D or arrays or cubemap

    TextureFilter minFilter{TextureFilter::Nearest};
    TextureFilter magFilter{TextureFilter::Nearest};

    TextureBorderColor borderColor{0, 0, 0, 0};
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
    std::vector<uint8_t> pixelsByLayers{}; // 2D pixels data, aligned 2D arrays pixels data, aligned 3D pixels data
    std::array<std::vector<uint8_t>, 6> pixelsByCubemap{}; // cubemap pixels data
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
    Span<ShaderEntryPoint> entryPoints;
    // Optional compile defines
    Span<const char*> defines;
    Span<std::pair<const char*, const char*>> variables;
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

using AttachmentFormat = TextureFormat;

struct AttachmentDesc {
    AttachmentFormat format;
    LoadOp load;
    StoreOp store;
    SampleCount samples{SampleCount::Sample1};
    bool hasResolve{false};
};

struct RenderPassDesc {
    Span<AttachmentDesc> colorAttachments;
    AttachmentDesc depthStencilAttachment;
    bool hasDepth{false};
    bool hasStencil{false};
};

// used if loadOp == Clear
struct RenderPassClear {
    float clearColor[4] = {0.0f,0.0f,0.0f,1.0f};
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;
};

struct RenderPassTag {};

using RenderPassHandle = ExternalHandle<RenderPassTag>;

struct FramebufferDesc {
    RenderPassHandle renderPass; // guides behavior (load/store, resolve, sampleCount)
    Span<TextureHandle> colorAttachments;

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
    TextureFormat format{TextureFormat::RGBA8_UINT};
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
    CullMode cull;
    FillMode fill;
    FrontFace frontFace;
    PolyMode polyMode;
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
    CompareOp compare;
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;

    // Operations
    StencilOp failOp;      // stencil fail
    StencilOp depthFailOp; // depth fail
    StencilOp passOp;      // pass
};

struct DepthStencilState {
    bool depthTest;
    bool depthWrite;
    CompareOp depthCompare;

    bool stencilTest; // optional
    StencilOpState stencilFront;
    StencilOpState stencilBack;
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
    bool enabled;

    BlendFactor srcColor;
    BlendFactor dstColor;
    BlendOp colorOp;

    BlendFactor srcAlpha;
    BlendFactor dstAlpha;
    BlendOp alphaOp;
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

struct VertexAttribute {
    uint32_t location;
    uint32_t componentCount;
    uint32_t divisor;
    uint32_t offset;
    uint32_t size;
    VertexTypeDesc typeDesc;
    bool normalized;
};

struct VertexLayout {
    uint32_t stride;
    Span<VertexAttribute> attributes;
};

struct RenderPipelineDesc {
    ShaderHandle shader;

    const char* vertexEntry;
    const char* fragmentEntry;

    RasterState raster;
    DepthStencilState depth;
    BlendState blend;
    VertexLayout layout;

    RenderPassHandle renderPass;
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
    Span<Binding> bindings{};

    uint32_t layoutID{0}; // layout compatibility id
    uint32_t setIndex{0}; // descriptor set index
    uint32_t version{1};  // increment on update
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

class ICommandList {
public:
    virtual ~ICommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void SetViewport(
        float x,
        float y,
        float width,
        float height,
        float minDepth = 0.0f,
        float maxDepth = 1.0f
    ) = 0;

    virtual void SetScissor(
        int32_t x,
        int32_t y,
        uint32_t width,
        uint32_t height
    ) = 0;

    virtual void BeginRenderPass(
        const RenderPassHandle& pass,
        const FramebufferHandle& framebuffer,
        const RenderPassClear& clear = {}
    ) = 0;

    virtual void EndRenderPass() = 0;

    virtual void BindRenderPipeline(const RenderPipelineHandle& handle) = 0;
    virtual void BindComputePipeline(const ComputePipelineHandle& handle) = 0;

    virtual void BindVertexBuffer(const BufferHandle& handle) = 0;
    virtual void BindIndexBuffer(const BufferHandle& handle) = 0;
    virtual void BindIndirectBuffer(const BufferHandle& handle) = 0;

    virtual void BindResourceSet(const ResourceSetHandle& res) = 0;

    virtual void Draw(
        uint32_t vertexCount,
        uint32_t firstVertex = 0
    ) = 0;

    virtual void DrawInstanced(
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex = 0
    ) = 0;

    virtual void DrawIndexed(
        uint32_t indexCount,
        uint32_t firstIndex = 0
    ) = 0;

    virtual void DrawIndexedInstanced(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex = 0
    ) = 0;

    virtual void DrawIndirect(
        uint32_t offset,
        uint32_t count,
        uint32_t stride
    ) = 0;

    virtual void DrawIndirectIndexed(
        uint32_t offset,
        uint32_t count,
        uint32_t stride
    ) = 0;
};

class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;

    virtual bool SupportsCap(GraphicsCapEnum cap) = 0;
    virtual const GraphicsCaps& GetCaps() const = 0;
    virtual utils::ExResult<GraphicsCaps> QueryCaps() = 0;

    virtual utils::ExResult<SwapchainHandle> CreateSwapchain(const SwapchainDesc& desc) = 0;
    virtual utils::AXError DestroySwapchain(const SwapchainHandle& desc) = 0;
    virtual utils::AXError ResizeSwapchain(const SwapchainHandle& desc, uint32_t width, uint32_t height) = 0;

    virtual utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) = 0;
    virtual utils::AXError UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data) = 0;
    virtual utils::AXError DestroyBuffer(const BufferHandle& handle) = 0;

    virtual utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) = 0;
    virtual utils::AXError UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data) = 0;
    virtual utils::AXError DestroyTexture(const TextureHandle& handle) = 0;

    virtual utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) = 0;
    virtual utils::AXError DestroyFramebuffer(const FramebufferHandle& handle) = 0;

    virtual utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc) = 0;
    virtual utils::AXError DestroyProgram(const ShaderHandle& handle) = 0;
    
    virtual utils::ExResult<RenderPipelineHandle> CreateRenderPipeline(const RenderPipelineDesc& desc) = 0;
    virtual utils::AXError DestroyRenderPipeline(const RenderPipelineHandle& handle) = 0;
    
    virtual utils::ExResult<ComputePipelineHandle> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual utils::AXError DestroyComputePipeline(const ComputePipelineHandle& handle) = 0;

    virtual utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) = 0;
    virtual utils::AXError DestroyRenderPass(const RenderPassHandle& handle) = 0;

    virtual utils::ExResult<ResourceSetHandle> CreateResourceSet(const ResourceSetDesc& desc) = 0;
    virtual utils::AXError UpdateResourceSet(const ResourceSetHandle& handle, Span<Binding> bindings) = 0;
    virtual utils::AXError DestroyResourceSet(const ResourceSetHandle& handle) = 0;

    virtual utils::AXError Execute(const ICommandList& cmd) = 0;
    virtual utils::AXError Dispatch(const ICommandList& cmdlist, uint32_t x, uint32_t y, uint32_t z) = 0;
    virtual utils::AXError Barrier(const ICommandList& cmdlist, Span<ResourceTransition> transitions) = 0;

    virtual utils::ExResult<uint32_t> AcquireNextImage() = 0;
    virtual utils::ExResult<FramebufferHandle> GetSwapchainFramebuffer(uint32_t imageIndex) = 0;
    virtual utils::AXError Present(uint32_t imageIndex) = 0;
};

class Graphics {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread{nullptr};
    UniquePtr<IGraphicsBackend> m_GfxBackend{nullptr};

    core::GfxType m_GfxType{core::GfxType::GL330};

    std::mutex m_GfxCapsMutex{};
public:
    explicit Graphics(SharedPtr<core::ThreadContextGfx> gfxThread);

    // Swapchain / presentation control
    void SetVSync(bool enabled);

    // Resource creation (thread-safe, async)
    Future<utils::ExResult<BufferHandle>> CreateBuffer(const BufferDesc& desc);
    Future<utils::AXError> UpdateBuffer(const BufferHandle& handle, size_t offset, size_t size, const void* data);
    Future<utils::AXError> DestroyBuffer(const BufferHandle& handle);

    Future<utils::ExResult<TextureHandle>> CreateTexture(const TextureDesc& desc);
    Future<utils::AXError> UpdateTexture(const TextureHandle& handle, const TextureSubDesc& subDesc, const void* data);
    Future<utils::AXError> DestroyTexture(const TextureHandle& handle);

    Future<utils::ExResult<FramebufferHandle>> CreateFramebuffer(const FramebufferDesc&);
    Future<utils::AXError> DestroyFramebuffer(const FramebufferHandle& handle);

    Future<utils::ExResult<ShaderHandle>> CreateProgram(const ShaderDesc& desc);
    Future<utils::AXError> DestroyProgram(const ShaderHandle& handle);

    Future<utils::ExResult<RenderPipelineHandle>> CreateRenderPipeline(const RenderPipelineDesc& desc);
    Future<utils::AXError> DestroyRenderPipeline(const RenderPipelineHandle& handle);

    Future<utils::ExResult<ComputePipelineHandle>> CreateComputePipeline(const ComputePipelineDesc& desc);
    Future<utils::AXError> DestroyComputePipeline(const ComputePipelineHandle& handle);

    Future<utils::ExResult<RenderPassHandle>> CreateRenderPass(const RenderPassDesc& desc);
    Future<utils::AXError> DestroyRenderPass(const RenderPassHandle& handle);

    Future<utils::ExResult<ResourceSetHandle>> CreateResourceSet(const ResourceSetDesc& desc);
    Future<utils::AXError> UpdateResourceSet(const ResourceSetHandle& handle, Span<Binding> bindings);
    Future<utils::AXError> DestroyResourceSet(const ResourceSetHandle& handle);

    // Command recording (thread-safe, CPU-side)
    SharedPtr<ICommandList> BeginCommandList();
    Future<utils::AXError> Submit(SharedPtr<ICommandList> cmdList);

    // Capabilities (cached, immutable)
    const GraphicsCaps& Capabilities();
};


}