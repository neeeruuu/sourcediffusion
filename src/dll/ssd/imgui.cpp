#include "callbacks.h"

#include "util/resources.h"

#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <imgui_notify.h>

#include "globals.h"

#include <d3d9.h>

bool imguiInitialized = false;
void initImGui()
{
    if (imguiInitialized)
        return;

    /*
        initialize / reset imgui
    */
    if (ImGui::GetCurrentContext())
    {
        ImGui::DestroyContext();
        ImGui_ImplWin32_Shutdown();
        ImGui_ImplDX9_Shutdown();
    }

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX9_Init(g_D3DDev);

    /*
      initialize notifications font
    */
    auto io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    float baseFontSize = 16.0f;
    float iconFontSize = baseFontSize * 2.0f / 3.0f;

    auto fontAwesomeS900 = Resource(g_DllMod, "SOLID900", "CUSTOMFONT");
    if (!fontAwesomeS900.data())
        throw;

    static const ImWchar iconsRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode = true;
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromMemoryTTF(fontAwesomeS900.data(), fontAwesomeS900.size(), iconFontSize, &iconsConfig,
                                   iconsRanges);

    imguiInitialized = true;
}

void drawImGui(IDirect3DDevice9* dvc)
{
    initImGui();

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

    g_CaptureInput = false;
    g_ImGuiCallback->run();

    CURSORINFO CursorInfo;
    CursorInfo.cbSize = sizeof(CursorInfo);
    if (GetCursorInfo(&CursorInfo) && CursorInfo.flags == 0)
        ImGui::GetIO().MouseDrawCursor = g_CaptureInput;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f));
    ImGui::RenderNotifications();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

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
auto imguiPresentListener = g_D3DPresentCallback->addListener(drawImGui);

void resetImGui(IDirect3DDevice9* dvc, D3DPRESENT_PARAMETERS* presentationParams) { imguiInitialized = false; }
auto imguiResetListener = g_D3DResetCallback->addListener(resetImGui);