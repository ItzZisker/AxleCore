#pragma once

#ifdef __AX_PLATFORM_WIN32__
#include "../AX_IRenderContext.hpp"

typedef void* HGLRC;
typedef void* HDC;
typedef void* HWND;

namespace axle::core {

class GL_RenderContextWin32 : public IRenderContext {
public:
    GL_RenderContextWin32();
    virtual ~GL_RenderContextWin32();

    bool Init(IApplication* app) override;
    void MakeCurrent() override;
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    void* GetContextHandle() const override;
private:
    HGLRC m_hglrc = nullptr;
    HDC   m_hdc = nullptr;
    HWND  m_hwnd = nullptr;
    bool m_Initialized = false;
};

}
#endif