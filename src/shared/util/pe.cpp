#include "pe.h"
#include "memory.h"

#include <corecrt_malloc.h>
#include <memoryapi.h>
#include <minwindef.h>
#include <winnt.h>

// 64kb granularity
// https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223
#define ALLOC_GRANULARITY 0x10000

unsigned __int64 PE::getSectionFromVA(unsigned __int64 imageBase, unsigned __int64 vaAddr)
{
    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(imageBase + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    unsigned __int64 sectionHeadersStart = imageBase + dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS);

    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sectionHeader =
            reinterpret_cast<PIMAGE_SECTION_HEADER>(sectionHeadersStart + (sizeof(IMAGE_SECTION_HEADER) * i));

        unsigned __int64 sectionVA = sectionHeader->VirtualAddress;

        if (vaAddr >= sectionHeader->VirtualAddress &&
            vaAddr <= sectionHeader->VirtualAddress + sectionHeader->SizeOfRawData)
            return reinterpret_cast<unsigned __int64>(sectionHeader);
    }

    return 0;
}

unsigned __int64 PE::getAddrFromVA(unsigned __int64 imageBase, unsigned __int64 vaAddr)
{
    PIMAGE_SECTION_HEADER section = reinterpret_cast<PIMAGE_SECTION_HEADER>(PE::getSectionFromVA(imageBase, vaAddr));
    if (section == nullptr)
        return 0;
    return imageBase + (section->PointerToRawData + (vaAddr - section->VirtualAddress));
}

/*
    same method as https://www.x86matthew.com/view_post?id=import_dll_injection
    but on x64 and using IMAGE_THUNK_DATA
*/
bool PE::attachDLLs(void* procHandle, unsigned __int64 mod, int dllCount, const char** dlls)
{
    // get headers
    IMAGE_DOS_HEADER dosHeader;
    if (!ReadProcessMemory(procHandle, reinterpret_cast<void*>(mod), &dosHeader, sizeof(IMAGE_DOS_HEADER), 0))
        return false;
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadProcessMemory(procHandle, reinterpret_cast<void*>(mod + dosHeader.e_lfanew), &ntHeaders,
                           sizeof(IMAGE_NT_HEADERS), 0))
        return false;
    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        return 0;

    // calculate size for new import directory and allocate
    int originalImportCount =
        ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
    int importDirSize = (originalImportCount + dllCount) * sizeof(IMAGE_IMPORT_DESCRIPTOR);

    PIMAGE_IMPORT_DESCRIPTOR idTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(malloc(importDirSize));
    if (!idTable)
        return false;

    // copy original imports to idt space reserved for new dlls
    unsigned long originalIDVA = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    unsigned long originalIDSize = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

    if (originalIDVA != 0 && originalIDSize != 0)
    {
        size_t readBytes;
        bool hasRead = ReadProcessMemory(procHandle, reinterpret_cast<void*>(mod + originalIDVA), &idTable[dllCount],
                                         originalIDSize, &readBytes);
        if (!hasRead || readBytes < originalIDSize)
            return false;
    }

    // calculate addr to use as base for remote allocations
    unsigned __int64 allocBase = mod;
    allocBase += ntHeaders.OptionalHeader.BaseOfCode;
    allocBase += ntHeaders.OptionalHeader.SizeOfCode;
    allocBase += ntHeaders.OptionalHeader.SizeOfInitializedData;
    allocBase += ntHeaders.OptionalHeader.SizeOfUninitializedData;

    // iterate all dlls to be loaded
    for (int i = 0; i < dllCount; i++)
    {
        // remotely allocate mem for dll path
        const char* dllPath = dlls[i];
        unsigned __int64 remoteDllPath = mem::allocNearBaseAddr(procHandle, mod, allocBase, strlen(dllPath) + 1,
                                                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!remoteDllPath)
            return false;

        if (!WriteProcessMemory(procHandle, reinterpret_cast<void*>(remoteDllPath), dllPath, strlen(dllPath) + 1, 0))
            return false;

        // setup thunk data, allocate and copy remotely
        IMAGE_THUNK_DATA thunkData[2];
#ifdef _AMD64_
        thunkData[0].u1.Ordinal = IMAGE_ORDINAL_FLAG64 + 1;
#else
        thunkData[0].u1.Ordinal = IMAGE_ORDINAL_FLAG32 + 1;
#endif
        thunkData[1].u1.Ordinal = 0;

        unsigned __int64 remoteITD = mem::allocNearBaseAddr(procHandle, mod, allocBase, sizeof(IMAGE_THUNK_DATA) * 2,
                                                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!remoteITD)
            return false;
        if (!WriteProcessMemory(procHandle, reinterpret_cast<void*>(remoteITD), thunkData, sizeof(IMAGE_THUNK_DATA) * 2,
                                0))
            return false;

        // alloc for IAT entry (same as ITD)
        unsigned __int64 remoteIAT = mem::allocNearBaseAddr(procHandle, mod, allocBase, sizeof(IMAGE_THUNK_DATA) * 2,
                                                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!remoteIAT)
            return false;
        if (!WriteProcessMemory(procHandle, reinterpret_cast<void*>(remoteIAT), thunkData, sizeof(IMAGE_THUNK_DATA) * 2,
                                0))
            return false;

        // define import descriptor and copy into import descriptor table
        IMAGE_IMPORT_DESCRIPTOR importDesc{};
        importDesc.OriginalFirstThunk = remoteITD - mod;
        importDesc.TimeDateStamp = 0;
        importDesc.ForwarderChain = 0;
        importDesc.Name = remoteDllPath - mod;
        importDesc.FirstThunk = remoteIAT - mod;

        memcpy(&idTable[i], &importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR));
    }

    // allocate and write idt to process
    unsigned __int64 idtRemote =
        mem::allocNearBaseAddr(procHandle, mod, allocBase, importDirSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!idtRemote)
        return false;
    if (!WriteProcessMemory(procHandle, reinterpret_cast<void*>(idtRemote), idTable, importDirSize, 0))
        return false;

    // update idt on nt headers to refer to new one
    ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = idtRemote - mod;
    ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = importDirSize;

    // write modified nt headers
    unsigned long oldHeaderProt;
    void* ntHeaderAddr = reinterpret_cast<void*>(mod + dosHeader.e_lfanew);
    if (VirtualProtectEx(procHandle, ntHeaderAddr, sizeof(IMAGE_NT_HEADERS), PAGE_READWRITE, &oldHeaderProt) == 0)
        return false;
    bool ntWritten = WriteProcessMemory(procHandle, ntHeaderAddr, &ntHeaders, sizeof(IMAGE_NT_HEADERS), 0);

    VirtualProtectEx(procHandle, ntHeaderAddr, sizeof(IMAGE_NT_HEADERS), oldHeaderProt, &oldHeaderProt);
    if (!ntWritten)
        return false;

    return true;
}