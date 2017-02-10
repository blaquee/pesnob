#ifndef _COMMON_ 
#define _COMMON_


typedef HMODULE(WINAPI *pLoadLibraryA)(LPCTSTR lpFileName);
typedef FARPROC(WINAPI *pGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);

typedef LPVOID(WINAPI *pVirtualAlloc)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
typedef BOOL(WINAPI *pVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
typedef void(__cdecl *stub_entry)();


#pragma pack(push, 1)
typedef struct _results
{
	int res_value;
	bool continuable;
	//char unused[2];
}results;

struct packed_section
{
	char name[8]; //Section name
	DWORD virtual_size; //Virtual size
	DWORD virtual_address; //Virtual address (RVA)
	DWORD size_of_raw_data; //Raw data size
	DWORD pointer_to_raw_data; //Raw data file offset
	DWORD characteristics; //Section characteristics
};

// This contains a linked list of stub entries. It contains pointers to all the stubs well be adding to the 
// target exe. Will be added to the pe_file_info structure
struct stub_entries {
	stub_entries* next;
	stub_entry ep;
};
struct pe_file_info
{
	short num_sections;
	//short num_stubs;
	DWORD size_packed;
	DWORD size_unpacked;
	
	DWORD total_virtual_size_of_sections; //Total virtual size of all original file sections 
	DWORD original_import_directory_rva; //Relative address of original import table
	DWORD original_import_directory_size; //Original import table size
	DWORD original_ep;
	//stub_entry* entries;

	pLoadLibraryA loadlib;
	pGetProcAddress getProcAddr;
	pVirtualAlloc vAlloc;
	pVirtualProtect vProtect;
	DWORD end_iat;
};
#pragma pack(pop)
#endif // 
