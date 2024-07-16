#pragma once

#include "util/keycodes.h"

#include "imguiextra.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <libloaderapi.h>

namespace ImGui
{
    namespace {
        typedef int (*tGetKeyNameTextA)(long lParam, char* lpString, int cchSize);
inline tGetKeyNameTextA GetKeyNameTextA;

typedef unsigned int (*tMapVirtualKeyA)(unsigned int uCode, unsigned int uMapType);
inline tMapVirtualKeyA MapVirtualKeyA;
    }
    inline void initKeybind()
    {
        static bool keybindInitialized = false;
        if (keybindInitialized)
            return;

        HMODULE user32 = LoadLibraryA("user32.dll");
        if (!user32)
            return;

        GetKeyNameTextA = reinterpret_cast<tGetKeyNameTextA>(GetProcAddress(user32, "GetKeyNameTextA"));
        MapVirtualKeyA = reinterpret_cast<tMapVirtualKeyA>(GetProcAddress(user32, "MapVirtualKeyA"));

        keybindInitialized = true;
    }

    /*
        TO-DO:
            keys like insert show as Num 0, fix
    */
    inline bool KeyBind(const char* label, unsigned int* keycode, bool* isFocused = nullptr)
    {
        initKeybind();

        ImGuiWindow* wnd = ImGui::GetCurrentWindow();
        ImGuiStyle* style = &ImGui::GetStyle();

        ImGuiContext& ctx = *GImGui;
        ImGuiIO& io = ctx.IO;

        if (wnd->SkipItems)
            return false;

        const ImVec2 labelSize = ImGui::CalcTextSize(label);
        const ImGuiID id = wnd->GetID(label);

        const float width = ImGui::CalcItemWidth();

        const ImRect totalBounds(wnd->DC.CursorPos - ImVec2(0, style->ItemInnerSpacing.y),
                                 wnd->DC.CursorPos + ImVec2(width, labelSize.y + (style->ItemInnerSpacing.y)));

        ImGui::ItemSize(totalBounds, style->FramePadding.y);
        if (!ImGui::ItemAdd(totalBounds, id, &totalBounds))
            return false;

        ImGui::RenderText(ImVec2(totalBounds.Min.x, ((totalBounds.Max.y + totalBounds.Min.y) / 2) - (labelSize.y / 2)),
                          label);

        char keyStr[15] = "Unbound";

        if (ImGui::GetActiveID() == id)
            strcpy_s(keyStr, "Waiting...");
        else if (*keycode)
        {
            unsigned int scanCode = MapVirtualKeyA(*keycode, 0);
            GetKeyNameTextA(scanCode << 16, keyStr, 10);
        }

        const ImVec2 keySize = ImGui::CalcTextSize(keyStr);

        const ImRect btnRect(
            ImVec2(totalBounds.Max.x - keySize.x - (style->ItemInnerSpacing.x * 2), totalBounds.Min.y),
            ImVec2(totalBounds.Max.x, totalBounds.Max.y));

        wnd->DrawList->AddRectFilled(btnRect.Min - ImVec2(style->ItemInnerSpacing.x, keySize.y * -.1f),
                                     btnRect.Max + ImVec2(style->ItemInnerSpacing.x, keySize.y * -.1f),
                                     ImColor(style->Colors[ImGuiCol_Button]));
        wnd->DrawList->AddText(((btnRect.Min + btnRect.Max) / 2) - ImVec2(keySize.x / 2, keySize.y / 2),
                               ImColor(style->Colors[ImGuiCol_Text]), keyStr);

        const bool hovered = ImGui::ItemHoverable(btnRect, id, 0);

        if (hovered && ImGui::IsMouseClicked(0, 0, id))
        {
            ImGui::SetActiveID(id, wnd);
            ImGui::SetFocusID(id, wnd);
            ImGui::FocusWindow(wnd);
        }

        bool valueChanged = false;

        if (isFocused)
            *isFocused = false;
        
        if (ImGui::GetActiveID() == id)
        {
            if (isFocused)
                *isFocused = true;

            for (int i = VK_BACK; i < VK_RMENU; i++)
            {
                if (io.KeysDown[i])
                {
                    *keycode = i;
                    valueChanged = true;
                    ImGui::ClearActiveID();
                    break;
                }
            }
        }

        return valueChanged;
    }
}