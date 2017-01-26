#ifndef _TYPEDEF_ 
#define _TYPEDEF_


typedef HMODULE(WINAPI *pLoadLibraryA)(LPCTSTR lpFileName);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);

typedef LPVOID(WINAPI *pVirtualAlloc)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
typedef BOOL(WINAPI *pVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
#endif // 
