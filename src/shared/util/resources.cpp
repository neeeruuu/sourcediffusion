#include "resources.h"

#include <windef.h>

#include <WinBase.h>

#include <libloaderapi.h>

Resource::Resource(void* module, const char* name, const char* type) : _size(0), _buff(nullptr)
{
    HMODULE modHandle = reinterpret_cast<HMODULE>(module);
    HRSRC info = FindResourceA(modHandle, name, type);
    if (!info)
        throw;

    HGLOBAL glob = LoadResource(modHandle, info);
    if (!glob)
        throw;

    _size = SizeofResource(modHandle, info);
    _buff = LockResource(glob);
}

Resource::~Resource()
{
    if (this->_buff)
        FreeResource(this->_buff);
}
