#include <Windows.h>
#include <winternl.h>
#include <intrin.h>
#include <stdbool.h>
#include "../LLVMStub/common.h"



	//forward declares
	extern int __strlen(const char* str);
	extern int __strncmp(const char* s1, const char* s2, unsigned int n);
	extern char* __strcpy(char* dst, const char* src);
	extern void * __cdecl memset(void *dst, int val, unsigned int count);
	extern void * __cdecl __memcpy(void * dst, const void * src, unsigned int count);
	void entrypoint(void* param, void* out);
	bool do_debugger_check();
	unsigned long caller(VOID);


	// Set section alignment to be nice and small
#pragma comment(linker, "/FILEALIGN:0x200")

//#pragma comment(linker,"/merge:.rdata=.data")
//#pragma comment(linker,"/merge:.data=.stub")
//#pragma comment(linker,"/merge:.reloc=.data")
	/*
	The packer will place some data before the bootstrap when the section is extracted,
	this is for the configuration structures. bootstrap will read those into local pointers and
	read/fill them accordingly, then pass information to the entry point

	Ultimately this is the technique that will be used for the actual decompressor stub, then that stub
	will directly call all subsequent stubs we add to the executable via the packer.
	*/
#pragma code_seg(".stub$a")
	void bootstrap()
	{
		unsigned long location;
		location = caller();

		// find the byte where 'results' is located, at this point its only used for
		// finding our position since pe_file_info may contain different information depending
		// on how that data structure is populated.
		while (TRUE)
		{
			if (*((byte *)location) == (byte)0x90)
				break;
			location--;
		}
		//get a pointer to the location of the result structure
		unsigned int res = (unsigned int)(location - sizeof(results) + 1);
		//get a pointer to the location of the pe_file_info structure
		unsigned int conf = (unsigned int)(res - sizeof(pe_file_info));
		

		//Get api's needed to bootstrap
		pe_file_info* peinfo = (pe_file_info*)conf;

		entrypoint(&conf, &res);
		results *ress = (results*)res;
		if (!ress->continuable)
		{
			//exit
		}
	}
#ifdef _WIN32
	__declspec(noinline) unsigned long caller(VOID) {
		return (unsigned long)_ReturnAddress();
	}
#else
	unsigned long __attribute__((noinline)) caller(VOID) {
		return  (ULONG_PTR)__builtin_return_address()
	}
#endif
	void entrypoint(void* param, void* out)
	{
		// void* addr = (void*)entrypoint;
		pe_file_info *myconf = (pe_file_info*)param;
		results *res = (results*)out;

		if (do_debugger_check())
		{
			// fill in our result parameters

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

