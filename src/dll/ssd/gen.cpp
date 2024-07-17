#include "gen.h"
#include "callbacks.h"
#include "globals.h"

#include <stable-diffusion.h>

#include "util/log.h"

#include <d3d9.h>

#include "source/classes/cviewsetup.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

#include <imgui.h>

#include <imgui_notify.h>

/*
    to-do:
        fix naming
        implement way to abort generation and mdl load (check if sd / ggml even allows that)
*/

GeneratorConfig genConfig{};
GeneratorState state = GENSTATE_INIT;

sd_ctx_t* ctx;
sd_image_t* img;

std::thread* generationThread = nullptr;
std::thread* loadThread = nullptr;

IDirect3DSurface9* rtCopy = nullptr;
IDirect3DTexture9* generatedTexture = nullptr;

bool regenContext()
{
    if (ctx)
    {
        free_sd_ctx(ctx);
        ctx = nullptr;
    }

    // #ifdef _DEBUG
    static const char* levels[]{"debug", "info", "warn", "error"};
    auto logCallback = [](sd_log_level_t lvl, const char* txt, void* data) {
        Log::verbose("[SD LOG ({})] {}", levels[lvl], txt);
    };
    sd_set_log_callback(logCallback, 0);

    auto progressCallback = [](int step, int steps, float time, void* data) {
        Log::verbose("[SD PROGRESS] Step: {}/{}, time: {}", step, steps, time);
    };
    sd_set_progress_callback(progressCallback, 0);
    // #endif

    int type = genConfig.type;
    if (type >= 4)
        type += 2; // q4_2 and q4_3 offset

    ctx = new_sd_ctx(genConfig.mdlPath, genConfig.vaePath, genConfig.taesdPath, genConfig.cnetPath, genConfig.loraPath,
                     "", "", false, false, false, -1, static_cast<sd_type_t>(genConfig.type),
                     static_cast<rng_type_t>(genConfig.rng), static_cast<schedule_t>(genConfig.schedule), false, false,
                     false);
    if (!ctx)
    {
        ImGuiToast toast(ImGuiToastType_Error, 3000);
        toast.set_title("Failed to load model");
        toast.set_content("new_sd_ctx returned nullptr");
        ImGui::InsertNotification(toast);

        Log::error("new_sd_ctx failed");
        return false;
    }

    ImGuiToast toast(ImGuiToastType_Success, 3000);
    toast.set_title("Model loaded");
    toast.set_content("Succesfully loaded model");
    ImGui::InsertNotification(toast);

    return true;
}

void generate(IDirect3DSurface9* rt)
{

    HRESULT res;

    D3DSURFACE_DESC rtDesc;
    res = rt->GetDesc(&rtDesc);
    if (FAILED(res))
    {
        Log::error("failed to get rt desc");
        return;
    }

    if (!rtCopy)
    {
        res = g_D3DDev->CreateRenderTarget(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DMULTISAMPLE_NONE, 0, true,
                                           &rtCopy, 0);
        if (FAILED(res))
        {
            Log::error("faild to create rt copy");
            return;
        }
    }

    res = g_D3DDev->StretchRect(rt, nullptr, rtCopy, nullptr, D3DTEXF_NONE);
    if (FAILED(res))
    {
        Log::error("failed to copy rt");
        return;
    }

    if (state != GENSTATE_IDLE || img)
        return;

    state = GENSTATE_GENERATING;

    if (generationThread)
    {
        if (generationThread->joinable())
            generationThread->join();
        delete generationThread;
        generationThread = nullptr;
    }

    int width = rtDesc.Width;
    int height = rtDesc.Height;
    if (genConfig.overrideResolution)
    {
        width = genConfig.width;
        height = genConfig.height;
    }

    uint8_t* buff = reinterpret_cast<uint8_t*>(malloc(width * height * 3));
    if (!buff)
        return;

    D3DLOCKED_RECT lockedRect;
    RECT srcRect{0, 0, width, height};
    res = rtCopy->LockRect(&lockedRect, &srcRect, D3DLOCK_READONLY);
    if (FAILED(res))
    {
        Log::error("failed to lock rtCopy");
        return;
    }

    for (int y = 0; y < height; y++)
    {
        uint8_t* row = buff + (y * width * 3);
        for (int x = 0; x < width; x++)
        {
            uint8_t* pixel = reinterpret_cast<uint8_t*>(lockedRect.pBits) + y * lockedRect.Pitch + x * 4;
            row[(x * 3) + 0] = pixel[2];
            row[(x * 3) + 1] = pixel[1];
            row[(x * 3) + 2] = pixel[0];
        }
    }

    rtCopy->UnlockRect();

    generationThread = new std::thread([width, height, buff] {
        sd_image_t src;
        src.width = width;
        src.height = height;
        src.channel = 3;
        src.data = buff;

        __int64 seed = genConfig.seed;
        if (seed == -1)
        {
            static std::random_device rd;
            static std::mt19937_64 gen(rd());
            static std::uniform_int_distribution<__int64> dist(LLONG_MIN, LLONG_MAX);
            seed = dist(gen);
        }

        img = img2img(ctx, src, genConfig.positivePrompt, genConfig.negativePrompt, -1, genConfig.cfg, src.width,
                      src.height, static_cast<sample_method_t>(genConfig.sampler), genConfig.steps, genConfig.strength,
                      seed, 1, 0, 0.f, .2f, false, "");

        state = GENSTATE_IDLE;

        free(buff);
    });
}

IDirect3DTexture9* getGeneratedTexture()
{
    if (img != nullptr)
    {

        if (generatedTexture)
        {
            generatedTexture->Release();
            generatedTexture = nullptr;
        }

        IDirect3DTexture9* newTex;
        g_D3DDev->CreateTexture(img->width, img->height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &newTex,
                                0);

        D3DLOCKED_RECT rect;
        newTex->LockRect(0, &rect, 0, 0);

        int w = img->width;
        int h = img->height;

        unsigned __int8* rectBuff = reinterpret_cast<unsigned __int8*>(rect.pBits);

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                int srcIdx = (y * w + x) * 3;
                int dstIdx = (y * w + x) * 4;

                rectBuff[dstIdx + 3] = 255;
                rectBuff[dstIdx + 2] = img->data[srcIdx + 0];
                rectBuff[dstIdx + 1] = img->data[srcIdx + 1];
                rectBuff[dstIdx + 0] = img->data[srcIdx + 2];
            }
        }

        newTex->UnlockRect(0);

        generatedTexture = newTex;

        free(img);
        img = nullptr;
    }

    return generatedTexture;
}

GeneratorConfig* generator::getConfig() { return &genConfig; }

GeneratorState generator::getState() { return state; }

void generator::loadModelWithSettings()
{
    if (state != GENSTATE_IDLE && state != GENSTATE_INIT)
        return;

    if (loadThread)
    {
        if (loadThread->joinable())
            loadThread->join();
        delete loadThread;
        loadThread = nullptr;
    }

    loadThread = new std::thread([] {
        state = GENSTATE_LOADING;
        regenContext();
        state = GENSTATE_IDLE;
    });

    loadThread->detach();
}

/*
    config stuff
*/
#define JSONGET(src, name, dst)                                                                                        \
    if (src.contains(name))                                                                                            \
        src.at(name).get_to(dst);

#define JSONGET_CHAR(src, name, dst)                                                                                   \
    if (src.contains(name))                                                                                            \
    {                                                                                                                  \
        std::string str = src[name].get<std::string>();                                                                \
        memcpy(dst, str.c_str(), str.length() + 1);                                                                    \
    }

void to_json(nlohmann::json& js, const GeneratorConfig& config)
{
    js["generator"] = {};
    js["generator"]["enabled"] = config.enabled;
    js["generator"]["draw_overlay"] = config.drawOverlay;
    js["generator"]["draw_source"] = config.drawSource;

    js["generator"]["model"] = config.mdlPath;
    js["generator"]["vae"] = config.vaePath;
    js["generator"]["taesd"] = config.taesdPath;
    js["generator"]["cnet"] = config.cnetPath;
    js["generator"]["lora"] = config.loraPath;

    js["generator"]["positive_prompt"] = config.positivePrompt;
    js["generator"]["negative_prompt"] = config.negativePrompt;

    js["generator"]["cfg"] = config.cfg;
    js["generator"]["steps"] = config.steps;
    js["generator"]["seed"] = config.seed;
    js["generator"]["strength"] = config.strength;

    js["generator"]["sampler"] = config.sampler;
    js["generator"]["rng"] = config.rng;
    js["generator"]["schedule"] = config.schedule;
    js["generator"]["type"] = config.type;

    js["generator"]["override_res"] = config.overrideResolution;
    js["generator"]["width"] = config.width;
    js["generator"]["height"] = config.height;

    js["generator"]["auto_save_cfg"] = config.autoSaveConfig;
}

void from_json(const nlohmann::json& js, GeneratorConfig& config)
{
    JSONGET(js["generator"], "enabled", config.enabled);
    JSONGET(js["generator"], "draw_overlay", config.drawOverlay);
    JSONGET(js["generator"], "draw_source", config.drawSource);

    JSONGET_CHAR(js["generator"], "model", config.mdlPath);
    JSONGET_CHAR(js["generator"], "vae", config.vaePath);
    JSONGET_CHAR(js["generator"], "taesd", config.taesdPath);
    JSONGET_CHAR(js["generator"], "cnet", config.cnetPath);
    JSONGET_CHAR(js["generator"], "lora", config.loraPath);

    JSONGET_CHAR(js["generator"], "positive_prompt", config.positivePrompt);
    JSONGET_CHAR(js["generator"], "negative_prompt", config.negativePrompt);

    JSONGET(js["generator"], "cfg", config.cfg);
    JSONGET(js["generator"], "steps", config.steps);
    JSONGET(js["generator"], "seed", config.seed);
    JSONGET(js["generator"], "strength", config.strength);

    JSONGET(js["generator"], "sampler", config.sampler);
    JSONGET(js["generator"], "rng", config.rng);
    JSONGET(js["generator"], "schedule", config.schedule);
    JSONGET(js["generator"], "type", config.type);

    JSONGET(js["generator"], "override_res", config.overrideResolution);
    JSONGET(js["generator"], "width", config.width);
    JSONGET(js["generator"], "height", config.height);

    JSONGET(js["generator"], "auto_save_cfg", config.autoSaveConfig);
}

void generator::loadConfig()
{
    std::string cfgPath = std::format("{}\\config.json", g_DllPath);
    if (!std::filesystem::exists(cfgPath))
        return;

    std::ifstream cfgFile(cfgPath);
    if (!cfgFile.is_open())
    {
        Log::error("could not open config file");
        return;
    }

    nlohmann::json js;
    try
    {
        js = nlohmann::json::parse(cfgFile);
        if (js.contains("generator"))
            js["generator"].get_to(genConfig);
    }
    catch (nlohmann::json::parse_error& ex)
    {
        Log::error("couldn't parse config file");
    }
}

void generator::saveConfig()
{
    nlohmann::json js;
    js["generator"] = genConfig;

    std::string configPath = std::format("{}\\config.json", g_DllPath);
    std::ofstream configFile(configPath);
    if (!configFile.is_open())
        return Log::error("failed to save config");

    configFile << js.dump(4);
    configFile.close();
}

/*
    callbacks
*/
void onPreRenderView(CViewSetup& viewRender)
{
    if (!genConfig.enabled)
        return;

    if (genConfig.overrideResolution)
    {
        viewRender.width = genConfig.width;
        viewRender.m_nUnscaledWidth = genConfig.width;

        viewRender.height = genConfig.height;
        viewRender.m_nUnscaledHeight = genConfig.height;

        viewRender.m_flAspectRatio = static_cast<float>(viewRender.width) / static_cast<float>(viewRender.height);
    }
}
auto rvListener = g_PreRenderViewCallback->addListener(onPreRenderView);

void onEnginePostProcessing()
{
    if (!genConfig.enabled)
        return;

    IDirect3DSurface9* rt;
    g_D3DDev->GetRenderTarget(0, &rt);

    if (rt)
        generate(rt);

    auto genTex = getGeneratedTexture();
    if (genTex)
    {
        IDirect3DSurface9* genSfc;
        genTex->GetSurfaceLevel(0, &genSfc);
        g_D3DDev->StretchRect(genSfc, 0, rt, 0, D3DTEXF_NONE);
        genSfc->Release();
    }
    rt->Release();
}
auto ppListener = g_PostEnginePostProcessingCallback->addListener(onEnginePostProcessing);

void drawOriginalSfc()
{
    if (!genConfig.drawSource)
        return;

    static IDirect3DTexture9* copyTex = nullptr;
    float uvX = 1.f;
    float uvY = 1.f;
    if (rtCopy)
    {
        HRESULT res;
        D3DSURFACE_DESC copyDesc{};
        rtCopy->GetDesc(&copyDesc);

        if (!copyTex)
        {
            res = g_D3DDev->CreateTexture(copyDesc.Width, copyDesc.Height, 1, D3DUSAGE_RENDERTARGET, copyDesc.Format,
                                     D3DPOOL_DEFAULT, &copyTex, 0);
            if (FAILED(res))
                return Log::error("failed to create texture of rt copy");
        }

        IDirect3DSurface9* copyTexSfc = nullptr;
        res = copyTex->GetSurfaceLevel(0, &copyTexSfc);
        if (FAILED(res))
            return Log::error("failed to get surface of copy texture");
        res = g_D3DDev->StretchRect(rtCopy, 0, copyTexSfc, 0, D3DTEXF_NONE);
        if (FAILED(res))
            return Log::error("failed to stretch rt copy");
        copyTexSfc->Release();

        if (genConfig.overrideResolution)
        {
            uvX = static_cast<float>(genConfig.width) / static_cast<float>(copyDesc.Width);
            uvY = static_cast<float>(genConfig.height) / static_cast<float>(copyDesc.Height);
        }
    }
    else
        copyTex = nullptr;

    ImGui::Begin("##Source", 0, ImGuiWindowFlags_NoTitleBar);
    auto wndSize = ImGui::GetContentRegionAvail();
    ImGui::Image(copyTex, wndSize, {0, 0}, {uvX, uvY});
    ImGui::End();
}
auto drawOriginalListener = g_ImGuiCallback->addListener(drawOriginalSfc);