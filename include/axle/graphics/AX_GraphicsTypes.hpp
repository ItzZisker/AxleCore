#pragma once

#include <cstdint>

namespace axle {

template<typename Tag>
struct GfxHandle {
    uint32_t index;
    uint32_t generation;
};

struct BufferTag {};
struct TextureTag {};
struct ShaderTag {};
struct PipelineTag {};
struct RenderPassTag {};

using BufferHandle     = GfxHandle<BufferTag>;
using TextureHandle    = GfxHandle<TextureTag>;
using ShaderHandle     = GfxHandle<ShaderTag>;
using PipelineHandle   = GfxHandle<PipelineTag>;
using RenderPassHandle = GfxHandle<RenderPassTag>;

}