#include "ssd/callbacks.h"
#include "ssd/globals.h"
#include "ssd/hooks.h"

#include "util/log.h"

#include <libloaderapi.h>

#include <safetyhook.hpp>

#include <d3d9.h>

#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

SafetyHookInline d3dPresentHook{};
SafetyHookInline d3dResetHook{};

bool imguiInitialized = false;
void initImGui(IDirect3DDevice9* dev)
{
    if (imguiInitialized)
        return;
    imguiInitialized = true;

    if (ImGui::GetCurrentContext())
    {
        ImGui::DestroyContext();
        ImGui_ImplWin32_Shutdown();
        ImGui_ImplDX9_Shutdown();
    }

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX9_Init(dev);
}

HRESULT hIDirect3DDevice9__Present(IDirect3DDevice9* dvc, const RECT* srcRect, const RECT* destRect,
                                   HWND destWndOverride, const RGNDATA* dirtyRegion)
{
    if (g_hWnd)
    {
        g_D3DDev = dvc;
        initImGui(dvc);

        g_CaptureInput = false;

        DWORD dwWrite;
        DWORD dwColorWrite;

        DWORD dwAddressU;
        DWORD dwAddressV;
        DWORD dwAddressW;
        DWORD dwTex;

        dvc->GetRenderState(D3DRS_SRGBWRITEENABLE, &dwWrite);
        dvc->GetRenderState(D3DRS_COLORWRITEENABLE, &dwColorWrite);

        dvc->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
        dvc->SetRenderState(D3DRS_COLORWRITEENABLE, 0xFFFFFFFF);

        dvc->GetSamplerState(NULL, D3DSAMP_ADDRESSU, &dwAddressU);
        dvc->GetSamplerState(NULL, D3DSAMP_ADDRESSV, &dwAddressV);
        dvc->GetSamplerState(NULL, D3DSAMP_ADDRESSW, &dwAddressW);
        dvc->GetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, &dwTex);

        dvc->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        dvc->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        dvc->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
        dvc->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, NULL);

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        g_D3DPresentCallback->run(dvc);

        CURSORINFO CursorInfo;
        CursorInfo.cbSize = sizeof(CursorInfo);
        if (GetCursorInfo(&CursorInfo))
            if (CursorInfo.flags == 0)
                ImGui::GetIO().MouseDrawCursor = g_CaptureInput;

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        dvc->SetRenderState(D3DRS_SRGBWRITEENABLE, dwWrite);
        dvc->SetRenderState(D3DRS_COLORWRITEENABLE, dwColorWrite);

        dvc->SetSamplerState(NULL, D3DSAMP_ADDRESSU, dwAddressU);
        dvc->SetSamplerState(NULL, D3DSAMP_ADDRESSV, dwAddressV);
        dvc->SetSamplerState(NULL, D3DSAMP_ADDRESSW, dwAddressW);
        dvc->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, dwTex);
    }

    return d3dPresentHook.call<HRESULT>(dvc, srcRect, destRect, destWndOverride, dirtyRegion);
}

HRESULT hIDirect3DDevice9__Reset(IDirect3DDevice9* dvc, D3DPRESENT_PARAMETERS* presentationParams)
{
    imguiInitialized = false;
    g_D3DResetCallback->run(dvc, presentationParams);
    return d3dResetHook.call<HRESULT>(dvc, presentationParams);
}

bool applyD3DHooks()
{
    HRESULT res;

    HMODULE d3dMod = LoadLibraryA("d3d9");
    if (!d3dMod)
        return false;

    auto d3dCreate = reinterpret_cast<IDirect3D9* (*)(unsigned int)>(GetProcAddress(d3dMod, "Direct3DCreate9"));

    IDirect3D9* d3d = d3dCreate(D3D_SDK_VERSION);
    if (!d3d)
        return false;

    D3DPRESENT_PARAMETERS pp;
    memset(&pp, '\0', sizeof(pp));
    pp.Windowed = true;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.hDeviceWindow = GetDesktopWindow();
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice9* dev = nullptr;

    res = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(),
                            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES, &pp, &dev);
    if (!FAILED(res))
    {
        unsigned __int64* vtable = *reinterpret_cast<unsigned __int64**>(dev);

        auto present = vtable[17];
        auto reset = vtable[16];

        dev->Release();
        d3d->Release();

        d3dPresentHook = safetyhook::create_inline(present, hIDirect3DDevice9__Present);
        if (!d3dPresentHook.enable())
        {
            Log::error("failed to hook present");
            return false;
        }

        d3dResetHook = safetyhook::create_inline(reset, hIDirect3DDevice9__Reset);
        if (!d3dResetHook.enable())
        {
            Log::error("failed to hook reset");
            return false;
        }

        return true;
    }

    Log::error("failed to create d3d device");

    return false;
}

bool removeD3DHooks()
{
    d3dPresentHook.reset();
    d3dResetHook.reset();
    return true;
}

HOOK_INFO(d3d, applyD3DHooks, removeD3DHooks);