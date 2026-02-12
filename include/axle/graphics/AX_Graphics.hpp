#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"
#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"
#include "slang.h"

#include <array>
#include <cstdint>
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
struct GfxHandle {
    uint32_t index;
    uint32_t generation;

    bool IsValid() const { return generation; }
};

enum class BufferUsage {
    BuVertex,
    BuIndex,
    BuUniform,
    BuStorage,
    BuIndirect,
    BuStaging
};

enum class BufferAccess {
    BLvlImmutable,
    BLvlDynamic,
    BLvlStream
};

struct BufferDesc {
    size_t size;
    BufferUsage usage;
    BufferAccess access;
    bool cpuVisible;
};

struct BufferTag {
    const BufferDesc desc;
};

using BufferHandle = GfxHandle<BufferTag>;

enum class TextureType {
    TxTTex2D,
    TxTTex3D,
    TxTCube,
    TxTArray2D
};

enum class TextureUsage : uint32_t {
    TxuNone            = 0,
    TxuSampled         = 1 << 0,
    TxuStorage         = 1 << 1,
    TxuRenderTarget    = 1 << 2,
    TxuDepthStencil    = 1 << 3,
    TxuTransferSrc     = 1 << 4,
    TxuTransferDst     = 1 << 5
};

enum class TextureFormat
{
    // 8-bit normalized
    TxFmtR8_UNORM,
    TxFmtR8_SNORM,
    TxFmtR8_UINT,
    TxFmtR8_SINT,

    TxFmtRG8_UNORM,
    TxFmtRG8_SNORM,
    TxFmtRG8_UINT,
    TxFmtRG8_SINT,

    TxFmtRGBA8_UNORM,
    TxFmtRGBA8_SNORM,
    TxFmtRGBA8_UINT,
    TxFmtRGBA8_SINT,

    TxFmtBGRA8_UNORM, // swapchain-friendly
    TxFmtRGBA8_SRGB,

    // 16-bit formats
    TxFmtR16_UNORM,
    TxFmtR16_SNORM,
    TxFmtR16_UINT,
    TxFmtR16_SINT,
    TxFmtR16_FLOAT,

    TxFmtRG16_UNORM,
    TxFmtRG16_SNORM,
    TxFmtRG16_UINT,
    TxFmtRG16_SINT,
    TxFmtRG16_FLOAT,

    TxFmtRGBA16_UNORM,
    TxFmtRGBA16_SNORM,
    TxFmtRGBA16_UINT,
    TxFmtRGBA16_SINT,
    TxFmtRGBA16_FLOAT,

    // 32-bit formats
    TxFmtR32_UINT,
    TxFmtR32_SINT,
    TxFmtR32_FLOAT,

    TxFmtRG32_UINT,
    TxFmtRG32_SINT,
    TxFmtRG32_FLOAT,

    TxFmtRGB32_FLOAT,
    TxFmtRGBA32_FLOAT,

    // Packed / special
    TxFmtRGB10A2_UNORM,
    TxFmtRG11B10_FLOAT,

    // Depth / Stencil
    TxFmtD16_UNORM,
    TxFmtD24_UNORM_S8_UINT,
    TxFmtD32_FLOAT,
    TxFmtD32_FLOAT_S8_UINT,

    // Block compression (BCn / S3TC)
    TxFmtBC1_UNORM,
    TxFmtBC1_SRGB,
    TxFmtBC3_UNORM,
    TxFmtBC3_SRGB,
    TxFmtBC4_UNORM,
    TxFmtBC4_SNORM,
    TxFmtBC5_UNORM,
    TxFmtBC5_SNORM,
    TxFmtBC6H_UFLOAT,
    TxFmtBC6H_SFLOAT,
    TxFmtBC7_UNORM,
    TxFmtBC7_SRGB,

    // ASTC (mobile)
    TxFmtASTC_4x4_UNORM,
    TxFmtASTC_4x4_SRGB,
    TxFmtASTC_6x6_UNORM,
    TxFmtASTC_6x6_SRGB,
    TxFmtASTC_8x8_UNORM,
    TxFmtASTC_8x8_SRGB,

    // Undefined
    TxFmtUnknown
};

inline bool TFmtIsCompressed(const TextureFormat& fmt) {
    return fmt > TextureFormat::TxFmtBC1_UNORM;
}

enum class TextureWrap {
    TwRepeat,             // GL_REPEAT / VK_SAMPLER_ADDRESS_MODE_REPEAT / D3D11_TEXTURE_ADDRESS_WRAP
    TwMirrorRepeat,       // GL_MIRRORED_REPEAT / VK_MIRROR_REPEAT / D3D11_TEXTURE_ADDRESS_MIRROR
    TwClampToEdge,        // GL_CLAMP_TO_EDGE / VK_CLAMP_TO_EDGE / D3D11_TEXTURE_ADDRESS_CLAMP
    TwClampToBorder       // GL_CLAMP_TO_BORDER / VK_CLAMP_TO_BORDER / D3D11_TEXTURE_ADDRESS_BORDER
};

enum class TextureFilter {
    TfNearest,            // GL_NEAREST / VK_FILTER_NEAREST / D3D11_FILTER_MIN_MAG_MIP_POINT
    TfLinear,             // GL_LINEAR / VK_FILTER_LINEAR / D3D11_FILTER_MIN_MAG_MIP_LINEAR
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

    TextureWrap wrapS{TextureWrap::TwRepeat};
    TextureWrap wrapT{TextureWrap::TwRepeat};
    TextureWrap wrapR{TextureWrap::TwRepeat}; // used for 3D or arrays or cubemap

    TextureFilter minFilter{TextureFilter::TfNearest};
    TextureFilter magFilter{TextureFilter::TfNearest};

    TextureBorderColor borderColor{0, 0, 0, 0};
};

struct TextureDesc {
    TextureType type;
    uint32_t width{1};
    uint32_t height{1};
    uint32_t depth{1}; // for 3D textures
    uint32_t layers{1}; // for 2D arrays, cube arrays

    TextureFormat format;
    TextureUsage usage;
    TextureSubDesc subDesc{};

    // Optional texture data
    std::vector<uint8_t> pixelsByLayers{}; // 2D pixels data, aligned 2D arrays pixels data, aligned 3D pixels data
    std::array<std::vector<uint8_t>, 6> pixelsByCubemap{}; // cubemap pixels data
};

struct TextureTag {};

using TextureHandle = GfxHandle<TextureTag>;

enum class ShaderStage {
    StgVertex,
    StgFragment,
    StgCompute,
    StgGeometry,
    StgHull,
    StgDomain
};

inline SlangStage StgToSlang(ShaderStage stg) {
    switch (stg) {
        case ShaderStage::StgVertex:   return SLANG_STAGE_VERTEX;
        case ShaderStage::StgFragment: return SLANG_STAGE_FRAGMENT;
        case ShaderStage::StgCompute:  return SLANG_STAGE_COMPUTE;
        case ShaderStage::StgGeometry: return SLANG_STAGE_GEOMETRY;
        case ShaderStage::StgHull:     return SLANG_STAGE_HULL;
        case ShaderStage::StgDomain:   return SLANG_STAGE_DOMAIN;
        default: return SLANG_STAGE_NONE;
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

using ShaderHandle = GfxHandle<ShaderTag>;

enum class ResourceKind : uint8_t {
    RkBuffer,
    RkTexture
};

struct ResourceHandle {
    ResourceKind kind;

    uint32_t index;
    uint32_t generation;

    ResourceHandle(BufferHandle h)
        : kind(ResourceKind::RkBuffer), index(h.index), generation(h.generation) {}
    ResourceHandle(TextureHandle h)
        : kind(ResourceKind::RkTexture), index(h.index), generation(h.generation) {}
};

enum class LoadOp {
    LOpLoad,       // keep previous contents
    LOpClear,      // clear at start
    LOpDontCare    // contents undefined
};

enum class StoreOp {
    SOpStore,
    SOpDiscard
};

using AttachmentFormat = TextureFormat;

struct AttachmentDesc {
    AttachmentFormat format;
    LoadOp load;
    StoreOp store;
    bool isDepth;
};

struct RenderPassDesc {
    Span<AttachmentDesc> colorAttachments;
    AttachmentDesc depthAttachment;
};

struct RenderPassTag {};

using RenderPassHandle = GfxHandle<RenderPassTag>;

struct FramebufferDesc {
    RenderPassHandle renderPass;
    Span<TextureHandle> colorAttachments;
    TextureHandle depthAttachment;
    uint32_t width;
    uint32_t height;
};

struct FramebufferTag {};

using FramebufferHandle = GfxHandle<FramebufferTag>;

enum class CullMode {
    CullNone,
    CullFront,
    CullBack
};

enum class FillMode {
    FillSolid,
    FillWireframe
};

enum class FrontFace {
    FFClockwise,
    FFCounterClockwise
};

struct RasterState {
    CullMode cull;
    FillMode fill;
    FrontFace frontFace;
};

enum class CompareOp {
    CmpOpNever,
    CmpOpLess,
    CmpOpEqual,
    CmpOpLessOrEqual,
    CmpOpGreater,
    CmpOpNotEqual,
    CmpOpGreaterOrEqual,
    CmpOpAlways
};

struct DepthStencilState {
    bool depthTest;
    bool depthWrite;
    CompareOp depthCompare;
    bool stencilTest; // optional, but future-proof
};

enum class BlendFactor {
    BlendZero,
    BlendOne,

    BlendSrcColor,
    BlendOneMinusSrcColor,

    BlendDstColor,
    BlendOneMinusDstColor,

    BlendSrcAlpha,
    BlendOneMinusSrcAlpha,

    BlendDstAlpha,
    BlendOneMinusDstAlpha,

    BlendConstantColor,
    BlendOneMinusConstantColor,

    BlendConstantAlpha,
    BlendOneMinusConstantAlpha
};

enum class BlendOp {
    BlendOpAdd,
    BlendOpSubtract,
    BlendOpReverseSubtract,
    BlendOpMin,
    BlendOpMax
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

struct PipelineDesc {
    ShaderHandle shader;

    const char* vertexEntry;
    const char* fragmentEntry;
    const char* computeEntry; // optional

    RasterState raster;
    DepthStencilState depth;
    BlendState blend;

    RenderPassHandle renderPass;
};

struct PipelineTag {};

using PipelineHandle = GfxHandle<PipelineTag>;

enum class BindingType {
    UniformBuffer,
    StorageBuffer,
    SampledTexture,
    StorageTexture,
    Sampler
};

struct Binding {
    uint32_t slot;
    BindingType type;
    ResourceHandle resource;
};

enum class BarrierType {
    Buffer,
    Texture
};

enum class VertexAttributeClass {
    VAcInt,
    VAcFloat,
    VAcDouble // Requires GL410
};

enum class VertexAttributeType {
    VAtInt8,
    VAtUInt8,
    VAtInt16,
    VAtUInt16,
    VAtInt32,
    VAtUInt32,
    VAtInt64, // Requires GL410
    VAtFloat16,
    VAtFloat32,
    VAtFloat64 // Requires GL410
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

struct VertexInputDesc {
    VertexLayout layout;
    Span<BufferHandle> vertexBuffers;
    BufferHandle indexBuffer; // optional, used if VertexInputDesc#index ~ true
    bool indexed{false};
};

struct VertexInputTag {};

using VertexInputHandle = GfxHandle<VertexInputTag>;

enum class GraphicsCapEnum {
    GCapComputeShaders,
    GCapShaderStorageBuffers,
    GCapIndirectDraw,
    GCapGeometryShaders,
    GCapTessellation,
    GCapMultiDrawIndirect,
    GCapTextureArray,
    GCapSparseTextures,
    GCapRayTracing,
    __Last__
};

struct GraphicsCaps {
    std::array<bool, (int)GraphicsCapEnum::__Last__ + 1> caps{};
    explicit GraphicsCaps() { caps.fill(false); }
};

class ICommandList {
public:
    ICommandList();
    virtual ~ICommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginRenderPass(RenderPassHandle) = 0;
    virtual void EndRenderPass() = 0;

    virtual void BindPipeline(PipelineHandle) = 0;
    virtual void BindVertexBuffer(BufferHandle) = 0;
    virtual void BindIndexBuffer(BufferHandle) = 0;

    virtual void Draw(uint32_t vertexCount, uint32_t firstVertex) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t firstIndex) = 0;
    virtual void DrawIndirect(BufferHandle indirect, uint32_t offset) = 0;
};

inline ResourceHandle GfxResource(BufferHandle h) { return ResourceHandle(h); }
inline ResourceHandle GfxResource(TextureHandle h) { return ResourceHandle(h); }

class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;

    virtual bool SupportsCap(GraphicsCapEnum cap) = 0;
    virtual utils::ExResult<GraphicsCaps> QueryCaps() = 0;

    virtual utils::ExResult<BufferHandle> CreateBuffer(const BufferDesc& desc) = 0;
    virtual utils::AXError UpdateBuffer(BufferHandle handle, size_t offset, size_t size, const void* data) = 0;
    virtual utils::AXError DestroyBuffer(BufferHandle& handle) = 0;

    virtual utils::ExResult<TextureHandle> CreateTexture(const TextureDesc& desc) = 0;
    virtual utils::AXError UpdateTexture(TextureHandle handle, const TextureSubDesc& subDesc, const void* data) = 0;
    virtual utils::AXError DestroyTexture(TextureHandle& handle);

    virtual utils::ExResult<FramebufferHandle> CreateFramebuffer(const FramebufferDesc& handle) = 0;
    virtual utils::AXError DestroyFramebuffer(FramebufferHandle handle) = 0;

    virtual utils::ExResult<ShaderHandle> CreateProgram(const ShaderDesc& desc) = 0;
    virtual utils::AXError DestroyProgram(ShaderHandle& handle) = 0;
    
    virtual utils::ExResult<PipelineHandle> CreatePipeline(const PipelineDesc& desc) = 0;
    virtual utils::AXError DestroyPipeline(PipelineHandle& handle) = 0;

    virtual utils::ExResult<VertexInputHandle> CreateVertexInput(const VertexInputDesc& desc) = 0;
    virtual utils::AXError DestroyVertexInput(VertexInputHandle& handle) = 0;

    virtual utils::ExResult<RenderPassHandle> CreateRenderPass(const RenderPassDesc& desc) = 0;

    virtual utils::AXError Dispatch(ICommandList& cmdlist, uint32_t x, uint32_t y, uint32_t z) = 0;
    virtual utils::AXError BindResources(ICommandList& cmdlist, Span<Binding> bindings) = 0;
    virtual utils::AXError Barrier(ICommandList& cmdlist, ResourceHandle handle) = 0;
};

class Graphics {
private:
    SharedPtr<core::ThreadContextGfx> m_GfxThread;
public:
    explicit Graphics(SharedPtr<core::ThreadContextGfx> gfxThread);
    ~Graphics();

    // Frame lifecycle (scheduled onto gfx thread)
    void BeginFrame();
    void EndFrame();

    // Swapchain / presentation control
    void SetVSync(bool enabled);

    // Resource creation (thread-safe, async)
    BufferHandle CreateBuffer(const BufferDesc& desc);
    void UpdateBuffer(BufferHandle handle, size_t offset, size_t size, const void* data);
    void DestroyBuffer(BufferHandle handle);

    TextureHandle CreateTexture(const TextureDesc& desc);
    void UpdateTexture(TextureHandle handle, uint32_t mip, const void* data);
    void DestroyTexture(TextureHandle handle);

    FramebufferHandle CreateFramebuffer(const FramebufferDesc&);
    void DestroyFramebuffer(FramebufferHandle);

    ShaderHandle CreateProgram(const ShaderDesc& desc);
    void DestroyProgram(ShaderHandle handle);

    PipelineHandle CreatePipeline(const PipelineDesc& desc);
    void DestroyPipeline(PipelineHandle handle);

    VertexInputHandle CreateVertexInput(const VertexInputDesc& desc);
    void DestroyVertexInput(VertexInputHandle& handle);

    RenderPassHandle CreateRenderPass(const RenderPassDesc& desc);

    // Command recording (thread-safe, CPU-side)
    ICommandList BeginCommandList();
    void Submit(ICommandList&& cmdList);

    // Capabilities (cached, immutable)
    const GraphicsCaps& Capabilities() const;
    void QueryCaps(GraphicsCaps& out);
};


}