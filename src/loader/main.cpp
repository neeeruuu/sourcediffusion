#include "util/log.h"
#include "util/pe.h"
#include "util/wininternals.h"

#include <handleapi.h>
#include <libloaderapi.h>
#include <memoryapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>

#include <processenv.h>
#include <winreg.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <regex>
#include <string>

// from: winerror.h
#define ERROR_SUCCESS 0L

// from: WinUser.h
#define MB_OK 0x00000000L
#define MB_ICONHAND 0x00000010L
#define MB_ICONERROR MB_ICONHAND

// from: WinBase.h
#define CREATE_SUSPENDED 0x00000004

#define GAME_NAME "GarrysMod"
#define EXE_NAME "bin/win64/gmod.exe"
#define DLL_NAME "source-diffusion.dll"
#define APP_ID "4000"

int reportError(const char* err)
{
    typedef int (*tMessageBoxA)(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);

    HMODULE user32 = LoadLibraryA("user32.dll");
    if (!user32)
        return 1;

    tMessageBoxA MessageBoxA = reinterpret_cast<tMessageBoxA>(GetProcAddress(user32, "MessageBoxA"));
    if (!MessageBoxA)
        return 1;
    MessageBoxA(0, err, "failed to load", MB_ICONERROR | MB_OK);

    return 1;
}

const char* getGamePath()
{
    char* gamePath = new char[MAX_PATH];
    /*
        find steam
    */
    char steamPath[MAX_PATH];
    HKEY steamKey;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steamKey) ==
        ERROR_SUCCESS)
    {
        DWORD keyLen = MAX_PATH;
        if (RegQueryValueExA(steamKey, "InstallPath", 0, 0, reinterpret_cast<LPBYTE>(&steamPath), &keyLen) ==
            ERROR_SUCCESS)
            steamPath[keyLen - 1] = '\0';
        else
            return 0;

        RegCloseKey(steamKey);
    }
    else
        return 0;

    std::string steamPathStr(steamPath);

    /*
        attempt the game's default path
    */

    std::string defaultGamePath = steamPathStr + "\\steamapps\\common\\" + GAME_NAME;
    if (std::filesystem::exists(defaultGamePath))
    {
        memcpy(gamePath, defaultGamePath.c_str(), MAX_PATH);
        return gamePath;
    }

    /*
        get all library folders
    */
    std::ifstream libConfig(steamPathStr + "\\steamapps\\libraryfolders.vdf");
    if (!libConfig.is_open())
        return 0;

    std::string configContent =
        std::string(std::istreambuf_iterator<char>(libConfig), std::istreambuf_iterator<char>());
    std::regex directoryRegex("\"[^\"]+\"[\\s]+\"([^\"]+)\"\\n", std::regex::ECMAScript);

    std::regex_iterator libraryFolders =
        std::sregex_iterator(configContent.begin(), configContent.end(), directoryRegex);

    /*
        iterate through all library folders and check if teardown is there
    */
    for (std::sregex_iterator match = libraryFolders; match != std::sregex_iterator(); ++match)
    {
        std::string gamePathStr = (*match)[1].str() + "\\steamapps\\common\\" + GAME_NAME;
        if (std::filesystem::exists(gamePathStr))
        {
            gamePathStr.replace(gamePathStr.find("\\\\"), 2, "\\");

            if (std::filesystem::exists(gamePathStr + "\\" + EXE_NAME))
            {
                memcpy(gamePath, gamePathStr.c_str(), MAX_PATH);
                return gamePath;
            }
        }
    }
    return 0;
}

bool attach(void* hProc, const char** dlls, int numDlls)
{
    typedef NTSTATUS (*tNtQueryInformationProcess)(HANDLE hProc, PROCESSINFOCLASS ProcInfoClass, PVOID ProcInfo,
                                                   ULONG ProcInfoLen, PULONG RetLen);
    tNtQueryInformationProcess NtQueryInformationProcess;

    /*
        get ntdll function
    */
    HMODULE ntdll = LoadLibraryA("ntdll.dll");
    if (!ntdll)
        return reportError("couldn't load ntdll.dll");
    NtQueryInformationProcess =
        reinterpret_cast<tNtQueryInformationProcess>(GetProcAddress(ntdll, "NtQueryInformationProcess"));

    /*
        Get process's basic information
    */
    PROCESS_BASIC_INFORMATION procInfo;
    ZeroMemory(&procInfo, sizeof(PROCESS_BASIC_INFORMATION));

    ULONG unused; // returnLength
    NtQueryInformationProcess(hProc, ProcessBasicInformation, &procInfo, sizeof(PROCESS_BASIC_INFORMATION), &unused);

    /*
        allocate and read PEB
    */
    PEB* peb = reinterpret_cast<PEB*>(malloc(sizeof(PEB)));
    if (!peb)
        return reportError("couldn't allocate memory for peb ptr");

    bool success = ReadProcessMemory(hProc, procInfo.PebBaseAddress, peb, sizeof(PEB), 0);
    if (!success)
        return reportError("couldn't read PEB");

    /*
        inject dll(s)
    */
    if (!PE::attachDLLs(hProc, reinterpret_cast<unsigned __int64>(peb->ImageBaseAddress), numDlls, dlls))
        return reportError("failed to attach dll(s)");

    /*
        cleanup
    */
    free(peb);

    return true;
}

int main(int argc, char* argv[])
{
    const char* gamePath = getGamePath();
    if (gamePath == 0)
        return reportError("failed to find game");

    std::string cmdLine = std::string(gamePath) + "\\" + EXE_NAME;
    cmdLine = cmdLine.append(" +gmod_mcore_test  0");
#ifdef _DEBUG
    cmdLine = cmdLine.append("+map gm_flatgrass +gamemode sandbox +maxplayers 32 -console");
#endif

    std::filesystem::path currentPath = std::filesystem::path(argv[0]).parent_path();
    std::string dllPath = currentPath.string() + "\\" + DLL_NAME;
    const char* dlls[]{
        dllPath.c_str(),
    };

    PROCESS_INFORMATION procInfo;
    STARTUPINFOA startupInfo;

    ZeroMemory(&procInfo, sizeof(procInfo));
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    SetEnvironmentVariableA("SteamAppId", APP_ID);
    if (!CreateProcessA(NULL, const_cast<LPSTR>(cmdLine.c_str()), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, gamePath,
                        &startupInfo, &procInfo))
        return reportError("failed to start process");

    attach(procInfo.hProcess, dlls, 1);

    ResumeThread(procInfo.hThread);

    WaitForSingleObject(procInfo.hProcess, -1);

    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    return 0;
}