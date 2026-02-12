#pragma once

#if defined(__AX_PLATFORM_WIN32__) && defined(__AX_GRAPHICS_DX11__)
#include "../AX_IRenderContext.hpp"

#include <wrl.h>

#include <d3d11.h>
#include <dxgi1_3.h>

using Microsoft::WRL::ComPtr;

typedef void* vHWND;

namespace axle::core {

class RenderContextDX11Win32 : public IRenderContext {
public:
    RenderContextDX11Win32();
    virtual ~RenderContextDX11Win32();

    bool Init(IApplication* app) override;
    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    void Shutdown() override;

    ComPtr<IDXGISwapChain> GetSwapChain() const;
    ComPtr<ID3D11RenderTargetView> GetRenderTargetView() const;
    ComPtr<ID3D11DeviceContext> GetDeviceContext() const;

    // Returns pointer to m_Device.Get();
    void* GetContextHandle() const override;
private:
    vHWND  m_hwnd = nullptr;
    bool m_Initialized = false;
    bool m_VSyncEnabled = false;

    ComPtr<ID3D11Device> m_Device = nullptr;
    ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
    ComPtr<IDXGISwapChain> m_SwapChain = nullptr;
    ComPtr<ID3D11RenderTargetView> m_RenderTargetView = nullptr;
};

}
#endif
