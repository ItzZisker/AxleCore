#pragma once

#include "axle/core/AX_GameLoop.hpp"
#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/utils/AX_Types.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <future>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace axle::graphics {

// GS ~ Graphics Synthesizer
union GS_HandleVal {
    int8_t* ptr;
    int32_t i32;
    int64_t i64;
    uint32_t ui32;
    uint64_t ui64;
    float_t f32;
}; // GS_HandleVal could be anything, could be an integer, float or any internal handle value.

enum GS_HandleEnum {
    GSH_Internal,
    GSH_Int32,
    GSH_Int64,
    GSH_UInt32,
    GSH_UInt64,
    //GSH_Float16,
    GSH_Float32,
    GSH_NullPtr
};
using GS_HandleVar = std::variant<
    int8_t*,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    float_t,
    std::nullptr_t
>;
using GS_Handle = std::pair<GS_HandleEnum, GS_HandleVal>;

GS_Handle GS_CreateHValNull();
GS_Handle GS_CreateHVal(GS_HandleEnum type, std::size_t raw);

GS_HandleVar GS_ReadHVar(GS_HandleEnum type, GS_HandleVal uni);
GS_HandleVar GS_ReadHVar(GS_Handle pair);

template <typename T>
inline T* GS_ReadHPtr(GS_Handle pair) { return reinterpret_cast<T*>(pair.second.ptr); }

inline GS_Handle GS_MakePair(GS_HandleEnum type, GS_HandleVal val) { return {type, val}; }

class GS_CommandPool;
class GS_CommandNode {
private:
    const bool m_IsTransient{false};
public:
    GS_CommandNode(bool transient = false) : m_IsTransient(transient) {}
    virtual ~GS_CommandNode() = default;
    
    virtual void Queue(const GS_CommandPool& pool) = 0;
    virtual GS_Handle Dispatch(SharedPtr<core::RenderThreadContext> ctx) const { return GS_CreateHVal(GSH_NullPtr, (size_t)nullptr); }

    bool IsTransient() const { return m_IsTransient; }
};

class GS_CommandPool {
private:
    explicit GS_CommandPool();
public:
    virtual ~GS_CommandPool() = default;

    template <typename T>
    requires std::is_base_of_v<GS_CommandPool, T>
    static inline std::future<SharedPtr<T>> EnqueueCreate(SharedPtr<core::RenderThreadContext> ctx) {
        return ctx->EnqueueFuture([ctx](){
            T* pool = new T();
            pool->Init(ctx);
            return SharedPtr<T>(pool);
        });
    }

    virtual void Init(SharedPtr<core::RenderThreadContext> ctx) = 0;
    virtual void Dispatch(SharedPtr<core::RenderThreadContext> ctx) = 0;

    virtual core::GraphicsBackend GetRendererEnum() const = 0;
    virtual GS_Handle GetHandle() const = 0;
};

template <typename T_CmdPool, typename T_Ret>
requires (std::is_base_of_v<GS_CommandPool, T_CmdPool> && !std::is_void_v<T_Ret>)
inline std::future<T_Ret> GS_PushWR(SharedPtr<core::RenderThreadContext> ctx, T_CmdPool cmd) {
    return ctx->EnqueueFuture([ctx, cmd = std::move(cmd)](){
        return cmd.Dispatch(ctx);
    });
}

template <typename T_CmdPool>
requires (std::is_base_of_v<GS_CommandPool, T_CmdPool>)
inline std::future<void> GS_PushNR(SharedPtr<core::RenderThreadContext> ctx, T_CmdPool cmd) {
    return ctx->EnqueueFuture([ctx, cmd = std::move(cmd)](){
        cmd.Dispatch(ctx);
    });
}

template <typename T_CmdNode, typename T_Ret>
requires (std::is_base_of_v<GS_CommandNode, T_CmdNode> && !std::is_void_v<T_Ret>)
inline std::future<T_Ret> GS_PushTWR(SharedPtr<core::RenderThreadContext> ctx, T_CmdNode cmd) {
    return ctx->EnqueueFuture([ctx, cmd = std::move(cmd)](){
        if (!cmd.IsTransient()) throw std::runtime_error("AX Exception: Non Transient Command!");
        return cmd.Dispatch(ctx);
    });
}

template <typename T_CmdNode>
requires (std::is_base_of_v<GS_CommandNode, T_CmdNode>)
inline std::future<void> GS_PushTNR(SharedPtr<core::RenderThreadContext> ctx, T_CmdNode cmd) {
    return ctx->EnqueueFuture([ctx, cmd = std::move(cmd)](){
        if (!cmd.IsTransient()) throw std::runtime_error("AX Exception: Non Transient Command!");
        cmd.Dispatch(ctx);
    });
}

enum GS_BufferType {
    GSB_Color = (1 << 0),
    GSB_Depth = (1 << 1),
    GSB_Stencil = (1 << 2)
};

};
