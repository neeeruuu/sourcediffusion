#include "log.h"

#ifdef _WIN32
    #include <consoleapi.h>
    #include <processenv.h>

    // from: WinBase.h
    #define STD_OUTPUT_HANDLE ((DWORD)-11)

void Log::createConsole()
{
    FILE* pstdout = stdout;
    AllocConsole();
    if (freopen_s(&pstdout, "CONOUT$", "w", stdout))
    {
        Log::error("failed to open console..");
    }
}

void Log::setupConsole()
{
    HANDLE conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD conMode = 0;
    GetConsoleMode(conHandle, &conMode);
    conMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(conHandle, conMode);
}
#endif

#include <iostream>

std::mutex mtx;
void Log::write(LogType type, std::string message)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << logTypeColors[type] << "[" << logTypeNames[type] << "]"
              << "\x1b[0m " << message << std::endl;
}