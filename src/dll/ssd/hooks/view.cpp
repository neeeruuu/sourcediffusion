#include "ssd/callbacks.h"
#include "ssd/globals.h"
#include "ssd/hooks.h"

#include "util/memory.h"

#include <safetyhook.hpp>

#include <libloaderapi.h>

#include "source/classes/cviewsetup.h"

SafetyHookInline ppHook{};
void hDoEnginePostProcessing(int x, int y, int w, int h, bool flashlightOn, bool postVGui)
{
    ppHook.call(x, y, w, h, flashlightOn, postVGui);
    g_EnginePostProcessingCallback->run();
}

bool applyViewHooks()
{
    auto client = GetModuleHandleA("client.dll");
    auto doEnginePostProcessing = mem::findPattern(
        client,
        "\xE8\x00\x00\x00\x00\x48\x85\xDB\x74\x09\x48\x8B\x03\x48\x8B\xCB\xFF\x50\x08\x48\x8B\x0D\x00\x00\x00\x00",
        "x????xxxxxxxxxxxxxxxxx????", true);
    HOOK(ppHook, doEnginePostProcessing, hDoEnginePostProcessing);

    return true;
}

bool removeViewHooks()
{
    // rvHook.reset();
    ppHook.reset();
    return true;
}

HOOK_INFO(view, applyViewHooks, removeViewHooks);
