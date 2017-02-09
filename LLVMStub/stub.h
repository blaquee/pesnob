#ifndef _STUB_
#define _STUB_


#define DEFAULT_VAL_BYTE 0xF
#define DEFAULT_VAL_WORD 0xFFFF
#define DEFAULT_VAL_DWORD 0xFFFFFFFF

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
