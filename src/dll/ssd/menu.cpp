#include "ssd/callbacks.h"
#include "ssd/gen.h"
#include "ssd/globals.h"
#include "ssd/imguicustom.h"

#include "util/bind.h"

#include <imfilebrowser.h>
#include <imgui.h>

#include <d3d9.h>

bool menuEnabled = false;
auto menuBind = new Bind(BIND_TOGGLE, VK_INSERT, &menuEnabled, true);

bool changingMenuBind = false;

void drawMenu()
{
    GeneratorConfig* cfg = generator::getConfig();
    GeneratorState state = generator::getState();

    if (cfg->autoSaveConfig)
    {
        static auto lastSave = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (now - lastSave > std::chrono::seconds(10))
        {
            generator::saveConfig();
            lastSave = now;
        }
    }

    if (changingMenuBind)
        menuBind->isActive = false;
    else
        menuBind->isActive = true;

    //if (!menuEnabled)
    //    return;

    g_CaptureInput = true;

    static bool isFirstFrame = true;
    auto io = ImGui::GetIO();
    static ImVec2 wndSize;
    static ImVec2 prevWndSize;

    if (ImGui::Begin("General", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Checkbox("Enabled", &cfg->enabled);
        ImGui::Checkbox("Draw overlay", &cfg->drawOverlay);
        ImGui::Checkbox("Draw original", &cfg->drawSource);

        ImGui::Separator();

        ImGui::Checkbox("Force resolution", &cfg->overrideResolution);

        static int width = cfg->width;
        if (ImGui::InputInt("width", &width))
            cfg->width = width;

        ImGui::SameLine();

        static int height = cfg->height;
        if (ImGui::InputInt("height", &height, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
            cfg->height = height;

        ImGui::KeyBind("menu key", &menuBind->key, &changingMenuBind);

        ImGui::Separator();

        if (ImGui::Button("save config"))
            generator::saveConfig();
        ImGui::SameLine();
        if (ImGui::Button("load config"))
            generator::loadConfig();

        ImGui::Checkbox("Automatically save", &cfg->autoSaveConfig);

        if (!isFirstFrame)
        {
            wndSize = ImGui::GetWindowSize();
            ImGui::SetWindowPos({io.DisplaySize.x - wndSize.x, io.DisplaySize.y - wndSize.y}, ImGuiCond_Once);
        }
        ImGui::End();
    }

    if (ImGui::Begin("Model", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Model: %s", cfg->mdlPath);
        ImGui::Text("VAE: %s", cfg->vaePath);
        ImGui::Text("lora: %s", cfg->loraPath);
        ImGui::Text("taesd: %s", cfg->taesdPath);

        static std::string selectedMdl(cfg->mdlPath);
        static ImGui::FileBrowser mdlDialog(0, selectedMdl.length() > 0
                                                   ? std::filesystem::path(selectedMdl).parent_path()
                                                   : std::filesystem::current_path());
        ImGui::Text("Model path:");
        ImGui::Text("%s", selectedMdl.substr(selectedMdl.find_last_of("/\\") + 1).c_str());
        if (ImGui::Button("Open##mdl"))
            mdlDialog.Open();
        ImGui::SameLine();
        if (ImGui::Button("Clear##mdl"))
            selectedMdl = "";

        static std::string selectedVAE(cfg->vaePath);
        static ImGui::FileBrowser vaeDialog(0, selectedVAE.length() > 0
                                                   ? std::filesystem::path(selectedVAE).parent_path()
                                                   : std::filesystem::current_path());
        ImGui::Text("VAE path:");
        ImGui::Text("%s", selectedVAE.substr(selectedVAE.find_last_of("/\\") + 1).c_str());
        if (ImGui::Button("Open##vae"))
            vaeDialog.Open();
        ImGui::SameLine();
        if (ImGui::Button("Clear##vae"))
            selectedVAE = "";

        static std::string selectedTaesd(cfg->taesdPath);
        static ImGui::FileBrowser taesdDialog(0, selectedTaesd.length() > 0
                                                     ? std::filesystem::path(selectedTaesd).parent_path()
                                                     : std::filesystem::current_path());
        ImGui::Text("taesd path:");
        ImGui::Text("%s", selectedTaesd.substr(selectedTaesd.find_last_of("/\\") + 1).c_str());
        if (ImGui::Button("Open##taesd"))
            taesdDialog.Open();
        ImGui::SameLine();
        if (ImGui::Button("Clear##taesd"))
            selectedTaesd = "";

        static std::string selectedCNet(cfg->cnetPath);
        static ImGui::FileBrowser cnetDialog(0, selectedCNet.length() > 0
                                                    ? std::filesystem::path(selectedCNet).parent_path()
                                                    : std::filesystem::current_path());
        ImGui::Text("cnet path:");
        ImGui::Text("%s", selectedCNet.substr(selectedCNet.find_last_of("/\\") + 1).c_str());
        if (ImGui::Button("Open##cnet"))
            cnetDialog.Open();
        ImGui::SameLine();
        if (ImGui::Button("Clear##cnet"))
            selectedCNet = "";

        static std::string selectedLora(cfg->loraPath);
        static ImGui::FileBrowser loraDialog(ImGuiFileBrowserFlags_SelectDirectory,
                                             selectedLora.length() > 0 ? std::filesystem::path(selectedLora)
                                                                       : std::filesystem::current_path());
        ImGui::Text("Lora path:");
        ImGui::Text("%s", selectedLora.substr(selectedLora.find_last_of("/\\") + 1).c_str());
        if (ImGui::Button("Open##lora"))
            loraDialog.Open();
        ImGui::SameLine();
        if (ImGui::Button("Clear##lora"))
            selectedLora = "";

        ImGui::Combo("rng type", &cfg->rng,
                     "std\0"
                     "cuda\0"
                     "\0");

        ImGui::Combo("scheduler", &cfg->schedule,
                     "default\0"
                     "discrete\0"
                     "karras\0"
                     "ays\0"
                     "\0");

        ImGui::Combo("type", &cfg->type,
                     "f32\0"
                     "f16\0"
                     "q4_0\0"
                     "q4_1\0"
                     "q5_0\0"
                     "q5_1\0"
                     "q8_0\0"
                     "q8_1\0"
                     "q2_k\0"
                     "q3_k\0"
                     "q4_k\0"
                     "q5_k\0"
                     "q6_k\0"
                     "q8_k\0"
                     "iq2_xxs\0"
                     "iq2_xs\0"
                     "iq3_xxs\0"
                     "iq1_s\0"
                     "iq4_nl\0"
                     "iq3_s\0"
                     "iq2_s\0"
                     "iq4_xs\0"
                     "i8\0"
                     "i16\0"
                     "i32\0"
                     "i64\0"
                     "f64\0"
                     "iq1_m\0"
                     "bf16\0"
                     //"automatic\0" // crashes with some models
                     "\0");

        bool canLoad = state == GENSTATE_INIT || state == GENSTATE_IDLE;
        if (!canLoad)
            ImGui::BeginDisabled();
        if (ImGui::Button("Load model"))
        {
            memcpy(cfg->mdlPath, selectedMdl.c_str(), selectedMdl.length() + 1);
            memcpy(cfg->loraPath, selectedLora.c_str(), selectedLora.length() + 1);
            memcpy(cfg->vaePath, selectedVAE.c_str(), selectedVAE.length() + 1);
            memcpy(cfg->taesdPath, selectedTaesd.c_str(), selectedTaesd.length() + 1);
            generator::loadModelWithSettings();
        }
        if (!canLoad)
            ImGui::EndDisabled();

        if (!isFirstFrame)
        {
            prevWndSize = wndSize;
            wndSize = ImGui::GetWindowSize();
            ImGui::SetWindowPos({io.DisplaySize.x - wndSize.x, io.DisplaySize.y - wndSize.y - prevWndSize.y},
                                ImGuiCond_Once);
        }

        ImGui::End();

        mdlDialog.Display();
        if (mdlDialog.HasSelected())
        {
            selectedMdl = mdlDialog.GetSelected().string();
            mdlDialog.ClearSelected();
        }

        vaeDialog.Display();
        if (vaeDialog.HasSelected())
        {
            selectedVAE = vaeDialog.GetSelected().string();
            vaeDialog.ClearSelected();
        }

        taesdDialog.Display();
        if (taesdDialog.HasSelected())
        {
            selectedTaesd = taesdDialog.GetSelected().string();
            taesdDialog.ClearSelected();
        }

        cnetDialog.Display();
        if (cnetDialog.HasSelected())
        {
            selectedCNet = cnetDialog.GetSelected().string();
            cnetDialog.ClearSelected();
        }

        loraDialog.Display();
        if (loraDialog.HasSelected())
        {
            selectedLora = loraDialog.GetSelected().string();
            loraDialog.ClearSelected();
        }
    }

    if (ImGui::Begin("Generation", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Prompt", cfg->positivePrompt, 1024);
        ImGui::InputText("Negative prompt", cfg->negativePrompt, 1024);
        ImGui::SliderFloat("Strength", &cfg->strength, 0.f, .9f);
        ImGui::SliderFloat("Cfg", &cfg->cfg, 0.f, 25.f);
        ImGui::SliderInt("Steps", &cfg->steps, 1, 50);
        ImGui::InputScalar("Seed", ImGuiDataType_S64, &cfg->seed);

        ImGui::Combo("sampler", &cfg->sampler,
                     "Euler A\0"
                     "Euler\0"
                     "Heun\0"
                     "DPM2\0"
                     "DPM++2S_a\0"
                     "DPM++2m\0"
                     "DPM++2mv2\0"
                     "lcm\0"
                     "\0");

        if (!isFirstFrame)
        {
            wndSize = ImGui::GetWindowSize();
            ImGui::SetWindowPos({0, io.DisplaySize.y - wndSize.y}, ImGuiCond_Once);
        }

        ImGui::End();
    }

    isFirstFrame = false;
}

auto menuListener = g_ImGuiCallback->addListener(drawMenu);