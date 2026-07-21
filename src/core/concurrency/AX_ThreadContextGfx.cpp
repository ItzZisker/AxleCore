#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/graphics/cmd/AX_IGraphicsBackend.hpp"

#include "axle/utils/AX_Universal.hpp"

namespace axle::core
{

utils::ExError ThreadContextGfx::StartGfx(CtxCreatorFunc initFunc, float frameCap, bool autoPresent) {
    m_IsAutoPresent.store(autoPresent);
    m_FrameCap.store(frameCap);
    
    using namespace std::chrono;

    auto constWk = [this]() {
        if (!m_Ctx) return;

        using namespace std::chrono;

        static thread_local auto frameStart = steady_clock::now();

        if (m_IsAutoPresent.load(std::memory_order_relaxed)) {
            auto img = m_Ctx->AcquireNextImage();

            if (img.has_value()) {
                auto perr = m_Ctx->Present(img.value());
                if (!perr.IsNoError()) std::cerr << "AX Error (AutoPresent): " << perr.GetMessage() << std::endl;
            } else {
                std::cerr << "AX Error (AcquireNextImage): " << img.error().GetMessage() << std::endl;
            }
        }

        auto now = steady_clock::now();
        auto delta = now - frameStart;

        float frameCap = m_FrameCap.load();

        if (frameCap > 0.0f) {
            auto targetFrame = nanoseconds((int64_t)((1.0 / frameCap) * 1e9));
            auto targetTime = frameStart + targetFrame;

            utils::Uni_NanoSleep(targetTime - steady_clock::now());

            now = steady_clock::now();
            delta = now - frameStart;
        }
        frameStart = steady_clock::now();

        float deltaF = duration<float>(delta).count();

        m_FrameTime.store(deltaF);
        m_AccumulatedTime.fetch_add(deltaF);
    };

    return StartSyncd(std::move(initFunc), std::move(constWk));
}

}