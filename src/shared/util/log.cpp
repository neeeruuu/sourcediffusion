#include "log.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>

std::mutex mtx;

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

std::ofstream outFile;
void Log::initOutput(const char* name, const char* location)
{
    if (outFile.is_open())
        return;

    try
    {
        std::filesystem::path p(location);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        throw "invalid path";
    }

    std::time_t t = std::time(nullptr);
    std::tm* localTime = std::localtime(&t);

    std::ostringstream fileNameStream;
    fileNameStream << name << " - " << std::put_time(localTime, "%Y-%m-%d %H.%M.%S") << ".txt";

    std::string fileName = fileNameStream.str();
    std::string filePath = std::format("{}\\{}", location, fileName);

    std::filesystem::create_directories(location);

    std::multimap<time_t, std::filesystem::directory_entry> logFiles;
    for (auto& entry : std::filesystem::directory_iterator(location))
    {
        time_t entryTime = entry.last_write_time().time_since_epoch().count();
        logFiles.insert(std::pair(entryTime, entry));
    }

    if (logFiles.size() >= LOG_MAX_ENTRIES)
    {
        size_t deleteCount = logFiles.size() - LOG_MAX_ENTRIES;

        int i = 0;
        for (auto const& [time, file] : logFiles)
        {
            i++;
            if (i > deleteCount)
                break;
            std::filesystem::remove(file);
        }
    }
    outFile.open(filePath, std::ios::out);
}

void Log::write(LogType type, std::string message)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << logTypeColors[type] << "[" << logTypeNames[type] << "]"
              << "\x1b[0m " << message << std::endl;

    if (outFile.is_open())
        outFile << "[" << logTypeNames[type] << "] " << message << std::endl;
}