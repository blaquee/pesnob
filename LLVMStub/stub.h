#ifndef _STUB_
#define _STUB_

extern DWORD oldEP;

#pragma pack(push,1)
typedef struct _config
{
	pLoadLibraryA pLoadLib;
	pGetProcAddress pGetProcAddr;
	pVirtualProtect pProtect;
	pVirtualAlloc pMemAlloc;
	//UINT magic; //unused
	DWORD baseAddress;
	DWORD oldEP;
	DWORD loader_addr; //back to the called loader
	stub_entry next;
}config;
#pragma pack(pop)

#endif // 
