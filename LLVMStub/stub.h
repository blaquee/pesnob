#ifndef _STUB_
#define _STUB_

extern DWORD oldEP;
typedef struct _config
{
	//pVirtualProtect pProtect;
	//pVirtualAlloc pMemAlloc;
	UINT magic; //unused
	DWORD baseAddress;
	DWORD oldEP;
	DWORD loader_addr; //back to the called loader
	stub_entry next;
}config;

#endif // 
