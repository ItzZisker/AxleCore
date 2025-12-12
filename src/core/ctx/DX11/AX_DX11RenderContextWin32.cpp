#include "axle/core/ctx/DX11/AX_DX11RenderContextWin32.hpp"

#if defined(__AX_GRAPHICS_DX11__) && defined(_WIN32) && defined(__AX_PLATFORM_WIN32__)
#include "AX_PCH.hpp"

#include <windows.h>

namespace axle::core {

DX11RenderContextWin32::DX11RenderContextWin32() = default;

DX11RenderContextWin32::~DX11RenderContextWin32() {
    Shutdown();
}

bool DX11RenderContextWin32::Init(IApplication* app) {
    if (m_Initialized) return true;

    m_hwnd = reinterpret_cast<HWND>(app->GetNativeWindowHandle());
    if (!m_hwnd) {
        std::cerr << "AX Error: DX11 requires valid HWND\n";
        return false;
    }

    RECT rect{};
    GetClientRect((HWND)m_hwnd, &rect);
    UINT width  = static_cast<UINT>(rect.right  - rect.left);
    UINT height = static_cast<UINT>(rect.bottom - rect.top);

    // --- 1. Create DXGI swapchain + device ---
    DXGI_SWAP_CHAIN_DESC scDesc{};
    scDesc.BufferDesc.Width  = width;
    scDesc.BufferDesc.Height = height;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count  = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount       = 1; // NOTE: For later, spec args add: Flip model requires >=2
    scDesc.OutputWindow      = (HWND)m_hwnd;
    scDesc.Windowed          = TRUE;
    scDesc.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD; // valid for D3D11 old style
    scDesc.Flags = 0;

    UINT deviceFlags = 0;

    D3D_FEATURE_LEVEL featureLevelsRequested[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL featureLevelCreated;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        deviceFlags,
        featureLevelsRequested,
        _countof(featureLevelsRequested),
        D3D11_SDK_VERSION,
        &scDesc,
        m_SwapChain.GetAddressOf(),
        m_Device.GetAddressOf(),
        &featureLevelCreated,
        m_DeviceContext.GetAddressOf()
    );

    if (FAILED(hr)) {
        std::cerr << "AX Error: D3D11CreateDeviceAndSwapChain failed! hr=" << std::hex << hr << "\n";
        return false;
    }

    // --- 2. Create Render Target View ---
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        std::cerr << "AX Error: Failed to get swapchain backbuffer\n";
        return false;
    }

    hr = m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_RenderTargetView.GetAddressOf());
    if (FAILED(hr)) {
        std::cerr << "AX Error: CreateRenderTargetView failed!\n";
        return false;
    }

    m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), nullptr);

    m_Initialized = true;
    return true;
}

void DX11RenderContextWin32::SwapBuffers() {
    if (!m_SwapChain) return;
    m_SwapChain->Present(m_VSyncEnabled ? 1 : 0, 0);
}

void DX11RenderContextWin32::SetVSync(bool enabled) {
    m_VSyncEnabled = enabled;
}

void DX11RenderContextWin32::Shutdown() {
    if (!m_Initialized) return;

    if (m_DeviceContext) {
        m_DeviceContext->ClearState();
        m_DeviceContext->Flush();
    }

    m_RenderTargetView.Reset();
    m_SwapChain.Reset();
    m_DeviceContext.Reset();
    m_Device.Reset();

    m_hwnd = nullptr;
    m_Initialized = false;
}

ComPtr<IDXGISwapChain> DX11RenderContextWin32::GetSwapChain() const {
    return this->m_SwapChain;
}

ComPtr<ID3D11RenderTargetView> DX11RenderContextWin32::GetRenderTargetView() const {
    return this->m_RenderTargetView;
}

ComPtr<ID3D11DeviceContext> DX11RenderContextWin32::GetDeviceContext() const {
    return this->m_DeviceContext;
}

void* DX11RenderContextWin32::GetContextHandle() const {
    // Return the ID3D11Device* as the context handle
    return reinterpret_cast<void*>(m_Device.Get());
}

}
#endif
