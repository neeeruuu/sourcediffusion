#pragma once

namespace PE {
	/*
		gets the pointer to a section header from a virtual address
	*/
	unsigned __int64 getSectionFromVA(unsigned __int64 imageBase, unsigned __int64 vaAddr);

	/*
		gets actual address from a virtual address
	*/
    unsigned __int64 getAddrFromVA(unsigned __int64 imageBase, unsigned __int64 vaAddr);

	/*
		adds dlls to import table of process
		(only works on new suspended processes)
	*/
	bool attachDLLs(void* procHandle, unsigned __int64 mod, int dllCount, const char** dlls);
}