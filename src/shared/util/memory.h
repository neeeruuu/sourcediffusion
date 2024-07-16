#pragma once

#include <vcruntime.h>

namespace mem
{
#ifdef _AMD64_
    /*
        get pointer to RIP relative address
    */
    __forceinline unsigned __int64 getPtr(unsigned __int64 dwAddress, unsigned __int32 iOffset = 0)
    {
        return dwAddress + *(__int32*)(dwAddress + iOffset) + iOffset + sizeof(__int32);
    }
    /*
        read RIP relative pointer
    */
    __forceinline unsigned __int64 readPtr(unsigned __int64 dwAddress, unsigned __int32 iOffset = 0)
    {
        return *reinterpret_cast<unsigned __int64*>(getPtr(dwAddress, iOffset));
    }

#endif

    /*
        checks if first 4 bytes match last 4 bytes of addr
    */
    __forceinline bool isUninitializedAddr(__int64 addr)
    {
        return static_cast<unsigned __int32>(addr >> 32) == static_cast<unsigned __int32>(addr & 0xFFFFFFFF);
    }

    /*
        scans a specific module for a pattern
    */
    intptr_t findPattern(void* modHandle, const char* pattern, const char* mask, bool isDirectRef = false);

    /*
        converts data to little endian
    */
    template <typename T> __forceinline void toLittleEndian(unsigned char* dest, T value)
    {
        for (size_t i = 0; i < sizeof(value); ++i)
            dest[i] = (value >> (8 * i)) & 0xFF;
    }

    /*
        remotely allocates memory after an address as near to base addr as possible
    */
    unsigned __int64 allocNearBaseAddr(void* procHandle, unsigned __int64 mod, unsigned __int64 base,
                                       unsigned long long size, unsigned long allocType, unsigned long protection);
}