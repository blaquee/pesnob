#include <Windows.h>
#include "typedefs.h"

#pragma section(".stub", read, execute, write)

struct _ptrs
{
	pVirtualProtect pProtect;
	pVirtualAlloc pMemAlloc;
	DWORD baseAddress;
};

__declspec(allocate(".stub"))
DWORD64 baseAddress = 0xBADF00DBAD00100;
