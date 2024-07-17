#include "interfaces.h"

#include <libloaderapi.h>

bool source::getInterfaces()
{
    auto matSysMod = GetModuleHandle("materialsystem.dll");
    if (!matSysMod)
        return false;

    auto matSysCreateInterface = reinterpret_cast<void* (*)(const char*, int*)>(GetProcAddress(matSysMod, "CreateInterface"));

    g_MatSys = reinterpret_cast<IMaterialSystem*>(matSysCreateInterface("VMaterialSystem080", NULL));
    if (!g_MatSys)
        return false;

    return true;
}
