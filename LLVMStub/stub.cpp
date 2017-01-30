#include <Windows.h>
#include <winternl.h>
#include <intrin.h>
#include "typedefs.h"
#include "stub.h"

#pragma section(".stub", read, execute, write)

#pragma data_seg(".studb")
config stub_config = {
	0x11223344,
	0x0BADF00D,
	0x0BADF00D,
	0x00000000,
};

__declspec(allocate(".stub"))
DWORD oldEP = 0x0BADF00D;

__declspec(allocate(".stub"))
DWORD h_kernel32 = NULL;

extern "C" {
#pragma code_seg(".crt")
	char* __strcpy(char* dst, const char* src)
	{
		if (!dst || !src)
			return nullptr;
		char* temp = dst;
		while ((*dst++ == *src++) != '\0')
			;
		return temp;
	}
	int __strlen(const char* str)
	{
		const char* tmp;
		if (!str)
			return -1;
		tmp = str;
		int count = 0;
		while (tmp++ != '\0')
			count++;
		return count;
	}
	int __strncmp(const char* s1, const char* s2, size_t n)
	{
		for (; n > 0; s1++, s2++, n--)
			if (*s1 != *s2)
				return((*(unsigned char*)s1 < *(unsigned char*)s2) ? -1 : 1);
			else if (*s1 == '\0')
				return 0;
		return 0;
	}
#pragma code_seg(".stub$a")
	bool do_debugger_check()
	{
		//x86 for now
		PPEB pPeb = (PPEB)__readfsdword(0x30);
		if (pPeb->BeingDebugged == 1)
			return true;
		return false;
	}
#pragma code_seg(".stub$b")
	void* stub_gpa(HMODULE base, const char* name)
	{
		PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
		UINT_PTR cbase = (UINT_PTR)base;

		if (dos->e_magic != IMAGE_DOS_SIGNATURE)
			ExitProcess(0); //dafug?

		PIMAGE_NT_HEADERS32 pe = (PIMAGE_NT_HEADERS32)(cbase + dos->e_lfanew);
		//UINT_PTR cpe = (UINT_PTR)pe;
		if (pe->Signature != IMAGE_NT_SIGNATURE)
			ExitProcess(0); //dafug?

		//Get Export Directory
		UINT_PTR pentry = (UINT_PTR)pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(cbase + pentry);

		PDWORD funcs = (PDWORD)(cbase + exports->AddressOfFunctions);
		PWORD ordinals = (PWORD)(cbase + exports->AddressOfNameOrdinals);
		PDWORD funcNames = (PDWORD)(cbase + exports->AddressOfNames);

		//ordinal lookup?
		if(HIWORD(name) == 0)
		{
			for(size_t i = 0; i < exports->NumberOfNames; i++)
			{
				if(ordinals[i] == LOWORD(name))
				{
					return (void*)(cbase + funcs[i]);
				}
			}
		}
		for(size_t i = 0; i < exports->NumberOfFunctions; i++)
		{
			UINT_PTR entry_point = funcs[i];
			if(entry_point)
			{
				//enumerate names
				for(size_t j = 0; j < exports->NumberOfNames; j++)
				{
					if(ordinals[j] == i && __strncmp(name, (const char*)(funcNames[j]+cbase), __strlen(name)) == 0)
					{
						return (void*)(cbase + entry_point);
					}
				}
			}
		}
		return 0;
	}

#pragma code_seg(".entry")
	void entrypoint(void* param, void* out)
	{
		//param is configuration
		_config *myconf = (_config*)param;
		//this should:
		// (a) find location in memory
		// (b) populate any pointers
		// (c) run stub code
		// (d) return to loader or call next stub entrypoint
		if (do_debugger_check())
		{
			// fill in out param, I dont like this method. we should find a solution to pass information
			results *res = (results*)out;
			res->continuable = false;
			res->res_value = 0;
		}
	}

}