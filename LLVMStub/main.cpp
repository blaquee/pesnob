#include <Windows.h>
#include <stdio.h>

#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "pe_lib/pe_bliss.h"
#include "common.h"
#include "helper.h"
#include "compress/lz4/lz4.h"

//#include "compress/lzz.h"

using namespace std;
using namespace pe_bliss;
//#pragma comment(lib, "lz.lib")

/*
	This function extracts the stub from the packer body, see main for future enhancements to this
*/
char* create_stub_blob(DWORD base_addr, const char** section_names, size_t num_sections, size_t *out_size)
{
	if (num_sections == 0)
		return 0;
	HMODULE thisMod = GetModuleHandle(nullptr);
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)thisMod;
	PIMAGE_NT_HEADERS32 nt = CALC_OFFSET(PIMAGE_NT_HEADERS32, dos, dos->e_lfanew);
	short numSections = nt->FileHeader.NumberOfSections;
	//naive approach, but w.e
	if (numSections < num_sections)
	{
		cout << "Where the hell is our stub" << endl;
		return 0;
	}
	string stub_data;
	stringstream ss;
	PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
	for (int i = 0; i < numSections; ++i)
	{
		for (int j = 0; j < num_sections; j++)
		{
			if (strcmp(section_names[j], (char*)sec[i].Name) == 0)
			{
				cout << "Section Address: " << hex << (DWORD)thisMod + sec[i].VirtualAddress << endl;
				char *data = (char*)(DWORD)thisMod + sec[i].VirtualAddress;
				//ensure we have the space
				stub_data.resize(sec[i].SizeOfRawData);
				memcpy(&stub_data[0], data, sec[i].SizeOfRawData);
				//trim the section
				strip_nullbytes(stub_data);
				ss << stub_data;
				stub_data.clear();
			}
		}

	}
	cout << "Total Stub Size: " << ss.str().size() << endl;
	char* ret = (char*)malloc(ss.str().size());
	*out_size = ss.str().size();
	memcpy(ret, const_cast<char*>(ss.str().data()), ss.str().size());
	return ret;
}

int main(int argc, char** argv)
{

	//Stub sections we want to extract
	const char *stubs[] = {
		".stub"	
	};

	// ignore, for debug purposes
	cout << "Size of results struct: " << sizeof(results) << endl;
	cout << "Size of pefileinfo: " << sizeof(pe_file_info) << endl;

	/* 
		Extract the stub from the target executable. Right now this is located in 
		the packers own body, however, the idea here is to change this to parse
		out this data from external executables written in the same manner as the code in 
		stub.cpp.

		It will take input as an exe and a configuration file that contains the sections to extract.
		The order of the sections is important if there are more than one to avoid any
		breakage in assembly branch operations. ( call rel16/32, jmp rel16/32 )

		config.ini
		[sections]
		name="stub"
		name="stub2"
		

		We can use any of several open source ini processors, or use json. Or simply use python to extract
		the binary blob
	*/
	size_t stub_size;
	char *ret = create_stub_blob(0, stubs, 1, &stub_size);
	if (stub_size == 0)
	{
		cout << "Couldn't find stub data!" << endl;
		return 0;
	}
	string new_stub(ret, stub_size);
	free(ret);

	// hardcoding the smallexe because I'm so fly
	char test_file2[] = { "E:\\Coding\\LLVMStub\\bin\\SmallExe.exe" };
	char test_file[] = { "C:\\git_code\\stubber\\bin\\SmallExe.exe" };

	ifstream target(test_file2, std::ios::in | std::ios::binary);
	if (!target)
	{
		cout << "Error Reading File" << endl;
		return 0;
	}

	try
	{
		// parse the pe file
		cout << "Parsing PE File" << endl;
		pe_base image(pe_factory::create_pe(target));
		// dotnet not supported (also add x64 when im not lazy)
		if (image.is_dotnet())
		{
			cout << "Unsupported File" << endl;
			return 0;
		}


		// Ensure we have a valid PE to pack.
		const auto& sections = image.get_image_sections();
		if (sections.empty())
		{
			cout << "No sections to compress" << endl;
			return 0;
		}

		// Prepare a new section for our stub
		cout << "Creating Packer Section" << endl;
		section packer_section;
		packer_section.readable(true).writeable(true).executable(true);
		packer_section.set_name("glpack");


		/*
			pe_file_info structure (see: common.h) contains information about the original executable
			and the packed exectuable. This structure is populated and then appended before 
			the decompression or loader stub. See stub.cpp to see how we get access to this.

			The full packer stub will eventually look like this:

			-----------------------
			pe_file_info structure |
			-----------------------|
			results structure      |
			-----------------------|
			decompressor stub      |
			-----------------------|
			stub 1                 |
			-----------------------|
			stub ... n             |
			-----------------------|


			The compressed file will be the first PE section, so the decompressor stub will know
			to grab the first section and decompress it and restore some data (such as original entrypoint rva)

			The packer will know what stubs to add by using the configuration files and command line
			parameters that have yet to be added. But stubs will be in a relative folder with their 
			respective configurations (this will allow automation and randomization of stub adding)
		*/

		pe_file_info peinfo = { 0 };
		peinfo.num_sections = image.get_number_of_sections();
		peinfo.original_ep = image.get_ep();
		peinfo.total_virtual_size_of_sections = image.get_size_of_image();

		// this string contains all the section table data from the original file
		string packed_section_info;
		{
			packed_section_info.resize(sections.size() * sizeof(packed_section));

			//section raw data
			string raw_data;
			unsigned int cur_section = 0;
			// iterate all the sections and populate the packed_section structure (NT_SECTION_HEADER)
			for (auto it = sections.begin(); it != sections.end(); ++it, ++cur_section)
			{
				const section &s = *it;
				{
					//building each section structure
					packed_section& info =
						reinterpret_cast<packed_section&>(packed_section_info[cur_section * sizeof(packed_section)]);

					info.characteristics = s.get_characteristics();
					info.pointer_to_raw_data = s.get_pointer_to_raw_data();
					info.size_of_raw_data = s.get_size_of_raw_data();
					info.virtual_address = s.get_virtual_address();
					info.virtual_size = s.get_virtual_size();
					memset(info.name, 0, sizeof(info.name));
					memcpy(info.name, s.get_name().c_str(), s.get_name().length());
				}
				//this section contains no data
				if (s.get_raw_data().empty())
					continue;
				raw_data += s.get_raw_data();
			}
			// no data in this pe file...
			if (raw_data.empty())
			{
				cout << "Empty sections" << endl;
				return 0;
			}
			packed_section_info += raw_data;
		}

		/*
			This section contains the original files sections and compresses them all together into
			one section for the new PE
		*/
		section packed_;
		packed_.set_name(".packd");
		packed_.readable(true).writeable(true).executable(true);
		string &out_buf = packed_.get_raw_data();

		// save the unpacked size.
		peinfo.size_unpacked = packed_section_info.size();

		// determine how large the compressed data will be
		size_t compressed_size = LZ4_compressBound(packed_section_info.size());
		cout << "LZ4 says it will return " << compressed_size << " bytes of compressed data" << endl;
		out_buf.resize(compressed_size);

		cout << "Packing sections" << endl;
		int rv = LZ4_compress_default(packed_section_info.data(), (char*)&out_buf[0], packed_section_info.size(), compressed_size);
		if (rv < 1)
		{
			cout << "Compression bad" << endl;
			return 0;
		}
		cout << "Compressed buffer contains " << out_buf.size() << " bytes of compressed data" << endl;
		cout << "LZ4 reported " << rv << endl;
		//encode_buf((unsigned char*)packed_section_info.data(), packed_section_info.size(), (unsigned char*)&out_buf[0], (unsigned long*)&compressed_size);
		//pithy_Compress(packed_section_info.data(), packed_section_info.size(), &out_buf[0], compressed_size, 1);


		// save the compressed data size
		peinfo.size_packed = compressed_size;
		
		// allocate some space for the results structure to add to the packer section
		// this actually served two purposes for now.
		// 1. To find the location of the pe_file_info structure and
		// 2. To write results of stub operations.
		results *res = (results*)malloc(sizeof(results));
		memset(res, 0x90, sizeof(results));

		//add the packed file structure to the beginning of packer stub
		string peinfo_buf(reinterpret_cast<const char*>(&peinfo), sizeof(peinfo));
		string resinfo_buf(reinterpret_cast<const char*>(res), sizeof(results));
		//out_buf = peinfo_buf + resinfo_buf + out_buf;

		new_stub = peinfo_buf + resinfo_buf + new_stub;
		// add the data to the (un)packer section
		packer_section.set_raw_data(new_stub);
		cout << "Aligned section size " << align_up(new_stub.size(), image.get_section_alignment());
		packer_section.set_size_of_raw_data(align_up(new_stub.size(), 0x200));
		free(res);

		{
			cout << "Recreating the executable" << endl;
			// Replace the first section with our packed section
			const section& first_section = image.get_image_sections().front();
			packed_.set_virtual_address(first_section.get_virtual_address());

			// Calculate the total size needed after the packed data is unpacked.
			const section& last_section = image.get_image_sections().back();
			DWORD total_virtual_size = last_section.get_virtual_address() +
				last_section.get_aligned_virtual_size(last_section.get_virtual_size()) -
				first_section.get_virtual_address();

			// delete current sections
			image.get_image_sections().clear();
			image.realign_file(0x200);

			// Add the new sections and set new EP.
			cout << "Adding encoded section " << endl;
			section &encoded_section = image.add_section(packed_);
			image.set_section_virtual_size(encoded_section, total_virtual_size);
			cout << "Adding unpacker section" << endl;
			section &packer_s = image.add_section(packer_section);

			// calculate the new entrypoint, which will be the bootstrap function inside the (un)packer section
			unsigned int new_ep = image.rva_from_section_offset(packer_s, peinfo_buf.size() + resinfo_buf.size());
			cout << "Setting new Entrypoint to rva: " << new_ep << endl;
			image.set_ep(new_ep);


			// Rebuild the import table for our packed executable.
			import_rebuilder_settings import_setting(true, false);
			import_library kern32;
			kern32.set_name("kernel32");

			imported_function funcs;
			funcs.set_name("LoadLibraryA");
			kern32.add_import(funcs);

			funcs.set_name("GetProcAddress");
			kern32.add_import(funcs);

			funcs.set_name("VirtualAlloc");
			kern32.add_import(funcs);

			funcs.set_name("VirtualProtect");
			kern32.add_import(funcs);

			// We want to write the import data to our pe_file_info structure so our stub can use it
			DWORD load_lib_rva = image.rva_from_section_offset(packer_s, offsetof(pe_file_info, loadlib));

			kern32.set_rva_to_iat(load_lib_rva);
			imported_functions_list imports_list;
			imports_list.push_back(kern32);

			import_setting.build_original_iat(false);
			import_setting.save_iat_and_original_iat_rvas(true, true);

			import_setting.set_offset_from_section_start(packer_s.get_raw_data().size());

			// we add the import to the packer section. (dont create a new section)
			rebuild_imports(image, imports_list, packer_s, import_setting);
		}

		//output new file
		std::string base_file_name(test_file);
		std::string::size_type slash_pos;
		if ((slash_pos = base_file_name.find_last_of("/\\")) != std::string::npos)
			base_file_name = base_file_name.substr(slash_pos + 1);

		base_file_name = "new_" + base_file_name;
		std::ofstream new_pe_file(base_file_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		if (!new_pe_file)
		{
			std::cout << "Cannot create " << base_file_name << std::endl;
			return -1;
		}
		cout << "Rebuilding PE file with new section" << endl;
		rebuild_pe(image, new_pe_file);
	}
	catch (const pe_exception &e)
	{
		cout << "Exception: " << e.what() << endl;
	}
	getchar();
	return 0;
}