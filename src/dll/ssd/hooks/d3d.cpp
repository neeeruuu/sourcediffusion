#include "ssd/callbacks.h"
#include "ssd/globals.h"
#include "ssd/hooks.h"

#include "util/log.h"

#include <libloaderapi.h>

#include <safetyhook.hpp>

#include <d3d9.h>

SafetyHookInline d3dPresentHook{};
SafetyHookInline d3dResetHook{};

HRESULT hIDirect3DDevice9__Present(IDirect3DDevice9* dvc, const RECT* srcRect, const RECT* destRect,
                                   HWND destWndOverride, const RGNDATA* dirtyRegion)
{
    if (g_hWnd)
    {
        g_D3DDev = dvc;
        g_D3DPresentCallback->run(dvc);
    }

    return d3dPresentHook.call<HRESULT>(dvc, srcRect, destRect, destWndOverride, dirtyRegion);
}

HRESULT hIDirect3DDevice9__Reset(IDirect3DDevice9* dvc, D3DPRESENT_PARAMETERS* presentationParams)
{
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

    res = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, GetDesktopWindow(),
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

    Log::error("failed to create d3d device ({:#010x})", res);

    return false;
}

bool removeD3DHooks()
{
    d3dPresentHook.reset();
    d3dResetHook.reset();
    return true;
}

HOOK_INFO(d3d, applyD3DHooks, removeD3DHooks);