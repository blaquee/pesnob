#include <Windows.h>
#include <winternl.h>
#include <intrin.h>
#include "../LLVMStub/common.h"

#pragma section(".stub", read, execute, write)

extern "C" {
	//forward declares
	int __strlen(const char* str);
	int __strncmp(const char* s1, const char* s2, size_t n);
	char* __strcpy(char* dst, const char* src);
	void * __cdecl _memset(void *dst, int val, unsigned int count);
	void * __cdecl _memcpy(void * dst, const void * src, unsigned int count);
	void entrypoint(void* param, void* out);
	bool do_debugger_check();


#pragma code_seg(".stub$a")
	void bootstrap()
	{
		void* location = bootstrap;
		void* res = (void*)((int*)location - sizeof(results));
		void* conf = (void*)((int*)res - sizeof(pe_file_info));

		/*
		__asm{
		pushad;
		}
		//find our location
		unsigned int location;
		__asm
		{
		lea eax, bootstrap
		mov location, eax
		}
		unsigned int res;
		__asm
		{
		mov ebx, location;
		mov ecx, SIZE results;
		sub ebx, ecx;
		sub ebx, 2;
		//mov edx, ebx;
		mov res, ebx;
		}
		//(void*)((void*)location - sizeof(results) + 2);
		unsigned int conf;
		//(void*)((void*)location - (void*)(res)-sizeof(pe_file_info));
		__asm
		{
		mov ebx, 1;
		}
		unsigned int original_base;
		unsigned int rva_first_section;

		//placeholder to fill in

		//unsigned int original_oep;
		//unsigned int base_addr;
		entrypoint(&conf, &res);

		// if there are more than one stubs to call the bootstrap will call the next stubs bootstrap after passing the configuration data
		__asm{
		popad;
		ret;
		}
		*/
		entrypoint(conf, res);
	}

	void entrypoint(void* param, void* out)
	{
		void* addr = (void*)entrypoint;
		pe_file_info *myconf = (pe_file_info*)param;
		results *res = (results*)out;
		//this should:
		// (a) find location in memory
		// (b) populate any pointers
		// (c) run stub code
		// (d) return to loader or call next stub entrypoint
		if (do_debugger_check())
		{
			// fill in our param, I dont like this method. we should find a solution to pass information

			res->continuable = false;
			res->res_value = 0;
		}
		else
		{
			res->continuable = true;
			res->res_value = 1;
		}
	}

	bool do_debugger_check()
	{
		//x86 for now
		PPEB pPeb = (PPEB)__readfsdword(0x30);
		if (pPeb->BeingDebugged == 1)
			return true;
		return false;
	}

	/*	void* stub_gpa(HMODULE base, const char* name)
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
	if (HIWORD(name) == 0)
	{
	for (size_t i = 0; i < exports->NumberOfNames; i++)
	{
	if (ordinals[i] == LOWORD(name))
	{
	return (void*)(cbase + funcs[i]);
	}
	}
	}
	for (size_t i = 0; i < exports->NumberOfFunctions; i++)
	{
	UINT_PTR entry_point = funcs[i];
	if (entry_point)
	{
	//enumerate names
	for (size_t j = 0; j < exports->NumberOfNames; j++)
	{
	if (ordinals[j] == i && __strncmp(name, (const char*)(funcNames[j] + cbase), __strlen(name)) == 0)
	{
	return (void*)(cbase + entry_point);
	}
	}
	}
	}
	return 0;
	}
	*/
	void * __cdecl _memset(
		void *dst,
		int val,
		unsigned int count
	)
	{
		void *start = dst;

		while (count--) {
			*(char *)dst = (char)val;
			dst = (char *)dst + 1;
		}

		return(start);
	}

	void * __cdecl _memcpy(
		void * dst,
		const void * src,
		unsigned int count
	)
	{
		void * ret = dst;

		/*
		* copy from lower addresses to higher addresses
		*/
		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst + 1;
			src = (char *)src + 1;
		}

		return(ret);
	}

	char* __strcpy(char* dst, const char* src)
	{
		if (!dst || !src)
			return 0;
		char* temp = dst;
		while ((*dst++ = *src++) != '\0')
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
}