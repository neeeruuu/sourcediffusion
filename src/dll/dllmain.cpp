#include "ssd/loader.h"

#include <libloaderapi.h>

// from: winnt.h
#define DLL_PROCESS_ATTACH 1

// empty export for iat injection
__declspec(dllexport) int __stdcall _(void) { return 0; }

bool DllMain(void* mod, unsigned long reason, void*)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            ssd::start(mod);
            DisableThreadLibraryCalls(reinterpret_cast<HMODULE>(mod));
            break;
    }

    return 1;
}