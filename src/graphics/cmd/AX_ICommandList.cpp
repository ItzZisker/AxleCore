#include "axle/graphics/cmd/AX_ICommandList.hpp"
#include "axle/graphics/cmd/GL/AX_GLCommandList.hpp"

#include "axle/graphics/cmd/AX_IGraphicsBackend.hpp"

#ifdef __AX_GRAPHICS_GL__
#include "axle/graphics/cmd/GL/AX_GLGraphicsBackend.hpp"
#endif

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/utils/AX_Expected.hpp"
#include "axle/utils/AX_Types.hpp"

#include <memory>
#include <mutex>

using namespace axle::utils;

namespace axle::gfx
{

SharedPtr<ICommandList> ICommandList::Create(SharedPtr<core::ThreadContextGfx> gfxThread) {
    auto ctx = gfxThread->GetContext();
    gfx::GfxType gfxType;

    if (ctx) {
        gfxType = ctx->GetContext()->GetType();
    } else {
        return SharedPtr<ICommandList>{nullptr};
    }
    switch (gfxType) {
        case GfxType::GL330:
#ifdef __AX_GRAPHICS_GL__
            return std::make_shared<GLCommandList>();
#else
            return SharedPtr<ICommandList>{nullptr};
#endif
        default: return SharedPtr<ICommandList>{nullptr}; // TODO: Vulkan, DirectX
    }
}

Future<ExError> ICommandList::Submit(SharedPtr<core::ThreadContextGfx> gfxThread) {
    return gfxThread->EnqueueFuture([this, gfxThread](){
        auto ctx = gfxThread->GetContext();
        if (!ctx) return ExError{"Context is null!"};
        return ctx->Execute(*this);
    });
}

}