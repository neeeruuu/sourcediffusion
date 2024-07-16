#include "ssd/globals.h"
#include "ssd/hooks.h"

#include <libloaderapi.h>

#include <safetyhook.hpp>

SafetyHookInline setPosHook{};
bool hSetCursorPos(int x, int y)
{
    if (g_CaptureInput)
        return true;
    return setPosHook.call<bool>(x, y);
}

bool applyCursorHooks()
{
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (!user32)
        return false;

    void* setCursorPosAddr = GetProcAddress(user32, "SetCursorPos");
    HOOK(setPosHook, setCursorPosAddr, hSetCursorPos);

    return true;
}

bool removeCursorHooks()
{
    setPosHook.reset();
    return true;
}

HOOK_INFO(cursor, applyCursorHooks, removeCursorHooks);