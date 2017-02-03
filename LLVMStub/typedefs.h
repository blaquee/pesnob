#ifndef _TYPEDEF_ 
#define _TYPEDEF_


typedef HMODULE(WINAPI *pLoadLibraryA)(LPCTSTR lpFileName);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);

typedef LPVOID(WINAPI *pVirtualAlloc)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
typedef BOOL(WINAPI *pVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
typedef void(__stdcall *stub_entry)(void* param, void* out);



//stub specific
extern "C" {
	extern void entrypoint(void* param, void* out);
	extern bool do_debugger_check();
}

typedef struct _results
{
	int res_value;
	bool continuable;
	char unused[2];
}results;


struct pe_file_info
{
	short num_sections;
	DWORD size_packed;
	DWORD size_unpacked;

	pLoadLibraryA loadlib;
	pGetProcAddress getProcAddr;
};
#endif // 
