#pragma once

#include "axle/core/ctx/AX_IRenderContext.hpp"
#include "axle/core/AX_GameLoop.hpp"
#include "axle/utils/AX_Types.hpp"

#include <cstddef>
#include <cstdint>
#include <future>
#include <type_traits>
#include <utility>
#include <variant>

namespace axle::gs {

// GS ~ Graphics Synthesizer
template <typename Tag>
struct Handle {
    uintptr_t value = 0;

    explicit operator bool() const { return value != 0; }
};

struct GLCommandPoolTag {};
using GLCommandPoolHandle = Handle<GLCommandPoolTag>;

using GS_Handle = std::variant<
    std::monostate,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    float,
    void*
>;

template <typename T>
T& GS_Get(GS_Handle& h) {
    return std::get<T>(h);
}

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

template <typename Cmd>
auto Push(SharedPtr<core::RenderThreadContext> ctx, Cmd&& cmdTemp)
-> std::future<decltype(cmdTemp.Dispatch(ctx))> {
    return ctx->EnqueueFuture(
        [ctx, cmd = std::forward<Cmd>(cmdTemp)]() mutable {
            return cmd.Dispatch(ctx);
        }
    );
}

enum GS_BufferType {
    GSB_Color = (1 << 0),
    GSB_Depth = (1 << 1),
    GSB_Stencil = (1 << 2)
};

};
