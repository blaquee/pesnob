#ifndef _TYPEDEF_ 
#define _TYPEDEF_


typedef HMODULE(WINAPI *pLoadLibraryA)(LPCTSTR lpFileName);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);

typedef LPVOID(WINAPI *pVirtualAlloc)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
typedef BOOL(WINAPI *pVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
typedef BOOL(WINAPI *pIsDebugger)(void); //not needed

//stub specific
typedef void(__stdcall *stub_entry)(void* param, void* out);

typedef struct _results
{
	int res_value;
	bool continuable;
	char unused[2];
}results;
#endif // 
