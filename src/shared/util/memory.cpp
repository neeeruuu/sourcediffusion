#include "memory.h"

#include <minwindef.h>

#include <Psapi.h>
#include <memoryapi.h>
#include <processthreadsapi.h>

// 64kb granularity
// https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223
#define ALLOC_GRANULARITY 0x10000

#define FOUR_GB 0xFFFFFFFF

/*
    compares the passed ptr against a signature
*/
bool patternCompare(const char* data, const char* patt, const char* mask)
{
    for (; *mask; ++mask, ++data, ++patt)
    {
        if (*mask == 'x' && *data != *patt)
            return false;
    }
    return true;
}

/*
    scans a specific module for a pattern
*/
intptr_t mem::findPattern(void* modHandle, const char* pattern, const char* mask, bool isDirectRef)
{
    /*
        TO-DO:
            use sections instead of scanning the whole image
    */
    MODULEINFO modInfo = {0};
    GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(modHandle), &modInfo, sizeof(MODULEINFO));

    DWORD64 baseAddr = reinterpret_cast<DWORD64>(modHandle);
    DWORD64 modSize = modInfo.SizeOfImage;

    for (unsigned __int64 i = 0; i < modSize; i++)
    {
        if (patternCompare(reinterpret_cast<char*>(baseAddr + i), pattern, mask))
        {
            if (isDirectRef)
            {
                auto curr = baseAddr + i;
                auto start = reinterpret_cast<intptr_t*>(baseAddr + i);

#if _WIN64
                return mem::getPtr(baseAddr + i, 1);
#else
                if (*reinterpret_cast<unsigned char*>(start) == 0xE8)
                    return curr + *(intptr_t*)(curr + 1) + 1 + sizeof(intptr_t);
                else
                    return *reinterpret_cast<intptr_t*>(baseAddr + i + 1);
#endif
            }
            return baseAddr + i;
        }
    }
    return 0;
}

/*
    remotely allocates memory after an address as near to base addr as possible
*/
unsigned __int64 mem::allocNearBaseAddr(void* procHandle, unsigned __int64 mod, unsigned __int64 base,
                                        unsigned long long size, unsigned long allocType, unsigned long protection)
{
    MEMORY_BASIC_INFORMATION mbi;

    unsigned __int64 lastAddr = base;
    for (;; lastAddr = reinterpret_cast<unsigned __int64>(mbi.BaseAddress) + mbi.RegionSize)
    {
        memset(&mbi, 0, sizeof(mbi));

        if (VirtualQueryEx(procHandle, reinterpret_cast<void*>(lastAddr), &mbi, sizeof(mbi)) == 0)
            break;

        if ((mbi.RegionSize & 0xFFF) == 0xFFF)
            break;

        if (mbi.State != MEM_FREE)
            continue;

        unsigned __int64 addr = reinterpret_cast<unsigned __int64>(mbi.BaseAddress);
        if (addr < base)
            continue;

        addr = addr + (ALLOC_GRANULARITY - 1) & ~(ALLOC_GRANULARITY - 1);

        DWORD64 dwFourGB = 0xFFFFFFFF;

        if ((addr + size - 1 - mod) > dwFourGB)
            return 0;

        for (; addr < reinterpret_cast<unsigned __int64>(mbi.BaseAddress) + mbi.RegionSize; addr += ALLOC_GRANULARITY)
        {
            void* allocated = VirtualAllocEx(procHandle, reinterpret_cast<void*>(addr), size, allocType, protection);
            if (!allocated)
                continue;

            if ((addr + size - 1 - mod) > dwFourGB)
                return 0;

            return reinterpret_cast<unsigned __int64>(allocated);
        }
    }

    return 0;
}
