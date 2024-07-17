#include "loader.h"
#include "globals.h"
#include "hooks.h"
#include "gen.h"

#include "util/log.h"

#include "source/interfaces.h"

#include <chrono>
#include <thread>

#include <libloaderapi.h>

#define ENSURE(cond, err)                                                                                              \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        Log::error(err);                                                                                               \
        ssd::shutdown();                                                                                               \
    }

void awaitModule(const char* name)
{
    void* mod = nullptr;
    auto start = std::chrono::steady_clock::now();
    while (!mod)
    {
        mod = GetModuleHandleA(name);

        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10))
        {
            Log::error("timed out while awaiting module: {}", name);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::thread* startThread;
void threadedStart()
{
    /*
        await modules used for hooks and load modules that spew to conout
    */
    awaitModule("engine.dll");
    awaitModule("client.dll");

    LoadLibraryA("vaudio_speex.dll");
    LoadLibraryA("libcef.dll");

    /*
        AllocConsole, open conout, enable ansi
    */
    Log::createConsole();
    Log::setupConsole();

    /*
        get dll path
    */
    char cModuleName[MAX_PATH];
    GetModuleFileNameA(reinterpret_cast<HMODULE>(g_DllMod), cModuleName, MAX_PATH);

    std::string sModuleName(cModuleName);
    std::string sModulePath = sModuleName.substr(0, sModuleName.find_last_of('\\'));

    memcpy(g_DllPath, sModulePath.c_str(), sModulePath.length() + 1);

    Log::info("SourceSD loading...");

    Log::info("getting source interfaces");
    ENSURE(source::getInterfaces(), "failed to get interfaces");

    Log::info("applying hooks");
    ENSURE(HookManager::get()->applyAll(), "couldn't apply all hooks");

    Log::info("loading config");
    generator::loadConfig();

    Log::info("ready");
}

void ssd::start(void* dllMod)
{
    g_DllMod = dllMod;
    startThread = new std::thread(threadedStart);
}

void ssd::shutdown()
{
    HookManager::get()->removeAll();
    Log::info("unloading");

    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(g_DllMod), 0);
}
