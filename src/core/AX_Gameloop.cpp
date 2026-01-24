#include "axle/core/AX_GameLoop.hpp"
#include "axle/core/app/AX_IApplication.hpp"

namespace axle::core
{

void GenericThreadContext::SubCycle() {
    m_SubCycle(*this);
}

void GenericThreadContext::StartCycle(int64_t mssleep, std::function<std::shared_ptr<AXGEmpty>()> initFunc, std::function<void(GenericThreadContext& gctx)> subCycle) {
    Start(mssleep, std::move(initFunc));
    this->m_SubCycle = std::move(subCycle);
}

void ApplicationThreadContext::SubCycle() {
    m_Ctx->PollEvents();
}

void ApplicationThreadContext::StartApp(std::function<std::shared_ptr<IApplication>()> initFunc) {
    Start(1, std::move(initFunc));
}

void RenderThreadContext::SubCycle() {
    m_Ctx->SwapBuffers(); // swapbuffers already introduce GPU-synch block which will push the GPU and CPU into battle and gives maximum framerate
}

void RenderThreadContext::StartGraphics(std::function<std::shared_ptr<IRenderContext>()> initFunc) {
    Start(0, std::move(initFunc)); // so, based on explanation above; mssleep becomes 0
}

}