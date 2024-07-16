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
    g_PostEnginePostProcessingCallback->run();
}

SafetyHookInline rvHook{};
class CViewRender;
void hCViewRender__RenderView(CViewRender* thisptr, CViewSetup& viewRender, int clearFlags, int whatToDraw)
{
    g_PreRenderViewCallback->run(viewRender);
    rvHook.call(thisptr, viewRender, clearFlags, whatToDraw);
}

bool applyViewHooks()
{
    auto client = LoadLibraryA("client.dll");
    auto renderView = mem::findPattern(client, "\x4C\x8B\xDC\x55\x53\x56\x41\x57\x49\x8D\xAB\x00\x00\x00\x00",
                                       "xxxxxxxxxxx????", false);
    auto doEnginePostProcessing = mem::findPattern(
        client,
        "\xE8\x00\x00\x00\x00\x48\x85\xDB\x74\x09\x48\x8B\x03\x48\x8B\xCB\xFF\x50\x08\x48\x8B\x0D\x00\x00\x00\x00",
        "x????xxxxxxxxxxxxxxxxx????", true);
    HOOK(rvHook, renderView, hCViewRender__RenderView);
    HOOK(ppHook, doEnginePostProcessing, hDoEnginePostProcessing);
    return true;
}

bool removeViewHooks()
{
    rvHook.reset();
    return true;
}

HOOK_INFO(view, applyViewHooks, removeViewHooks);