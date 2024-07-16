#include "ssd/callbacks.h"
#include "ssd/gen.h"

#include "imguiextra.h"
#include <imgui.h>

#include <d3d9.h>

#include <comdef.h>
#include <WbemCli.h>

#include <stable-diffusion.h>

void drawOverlay(IDirect3DDevice9* dev)
{
    ImVec2 cursor(40, 20);
    auto drawList = ImGui::GetBackgroundDrawList();

    auto addText = [drawList, &cursor](std::string txt) {
        drawList->AddText(cursor + ImVec2(0, 1), ImColor(0, 0, 0), txt.c_str());
        drawList->AddText(cursor + ImVec2(0, -1), ImColor(0, 0, 0), txt.c_str());
        drawList->AddText(cursor + ImVec2(1, 0), ImColor(0, 0, 0), txt.c_str());
        drawList->AddText(cursor + ImVec2(-1, 0), ImColor(0, 0, 0), txt.c_str());

        drawList->AddText(cursor, ImColor(255, 255, 255), txt.c_str());

        cursor.y += ImGui::CalcTextSize(txt.c_str()).y;
    };

    auto cfg = generator::getConfig();

    addText(std::format("SourceDiffusion - {}", SOURCEDIFFUSION_VER));
    addText("");
    addText(std::format("enabled: {}", cfg->enabled));
    addText(std::format("state: {}", genStateStr[generator::getState()]));
    addText("");
    addText(std::format("override res: {}", cfg->overrideResolution));
    addText(std::format("res: {} x {}", cfg->width, cfg->height));
    addText("");

    static auto sysInfo = sd_get_system_info();

    addText(sysInfo);
}

auto overlayListener = g_D3DPresentCallback->addListener(drawOverlay);