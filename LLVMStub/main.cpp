#include <stdio.h>
#include <Windows.h>

#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "common.h"
#include "stub.h"
#include "helper.h"
#include "pe_lib\pe_bliss.h"
#include "compress\pithy.h"

using namespace std;
using namespace pe_bliss;


DWORD get_original_entrypoint()
{
	HMODULE module = GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER pImageDosHeader = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS32 pImageNtHeaders = CALC_OFFSET(PIMAGE_NT_HEADERS32, pImageDosHeader, pImageDosHeader->e_lfanew);
	return pImageNtHeaders->OptionalHeader.AddressOfEntryPoint;
}

// Copy the stub sections, order matters
char* create_stub_blob(const char** section_names, size_t num_sections, size_t *out_size)
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
				cout << "Section Address: "<< hex << (DWORD)thisMod + sec[i].VirtualAddress << endl;
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
	//Stub sections we want to copy
	const char *stubs [] = {
		".stub",
		".crt"
	};

	//extract stub data from packer body
	size_t stub_size;
	char *ret = create_stub_blob(stubs, 2, &stub_size);
	if (stub_size == 0)
	{
		cout << "Couldn't find stub data!" << endl;
		return 0;
	}
	string new_stub(ret,stub_size);
	free(ret);

	config stub1{
		0
	};

	results *out = (results*)malloc(sizeof(results));
	memset(out, 0, sizeof(results));

	char test_file[] = { "C:\\git_code\\stubber\\Release\\SmallExe.exe" };

	ifstream target(test_file, std::ios::in | std::ios::binary);
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
		// dotnet not supported
		if (image.is_dotnet())
		{
			cout << "Unsupported File" << endl;
			return 0;
		}


		const auto& sections = image.get_image_sections();
		if (sections.empty())
		{
			cout << "No sections to compress" << endl;
			return 0;
		}
		//create the packer section
		section packer_section;
		packer_section.readable(true).writeable(true).executable(true);
		packer_section.set_name("glpack");
		packer_section.set_raw_data(new_stub);
		packer_section.set_size_of_raw_data(align_up(new_stub.size(), image.get_section_alignment()));

		//section &added_section_stub = image.add_section(packer_section);
		//image.set_section_virtual_size(added_section_stub, 0x1000);

		pe_file_info peinfo = { 0 };
		peinfo.num_sections = image.get_number_of_sections();
		peinfo.original_ep = image.get_ep();
		peinfo.total_virtual_size_of_sections = image.get_size_of_image();

		string packed_section_info;
		{
			packed_section_info.resize(sections.size() * sizeof(packed_section));

			//section raw data
			string raw_data;
			unsigned int cur_section = 0;
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
				if (s.get_raw_data().empty())
					continue;
				raw_data += s.get_raw_data();
				}
			if(raw_data.empty())
			{
				cout << "Empty sections" << endl;
				return 0;
			}
			packed_section_info += raw_data;
		}

		section packed_;
		packed_.set_name(".packd");
		packed_.readable(true).writeable(true).executable(true);
		string &out_buf = packed_.get_raw_data();



		peinfo.size_unpacked = packed_section_info.size();
		//char* buf_out = malloc(packed_section_info.size()*2);
		size_t compressed_size;

		cout << "Packing sections" << endl;
		pithy_Compress(packed_section_info.data(), packed_section_info.size(), &out_buf[0], compressed_size, 1);

		if (compressed_size == 0)
		{
			cout << "Encoding failed " << endl;
			return 0;
		}

		//store size of packed data
		peinfo.size_packed = compressed_size;
		//string encoded_buffer(buf_out, new_size);

		//add the packed file structure to the beginning of encoded buffer
		out_buf = string(reinterpret_cast<const char*>(&peinfo), sizeof(peinfo)) + out_buf;
		//out_buf = encoded_buffer;
		//calculating enough space for the encoded section (when unpacked)
		{
			cout << "Recreating the executable" << endl;
			const section& first_section = image.get_image_sections().front();
			packed_.set_virtual_address(first_section.get_virtual_address());

			const section& last_section = image.get_image_sections().back();
			DWORD total_virtual_size = last_section.get_virtual_address() +
				last_section.get_aligned_virtual_size(last_section.get_virtual_size()) -
				first_section.get_virtual_address();

			// delete current sections
			image.get_image_sections().clear();

			image.realign_file(0x200);
			
			//add the sections
			cout << "Adding encoded section " << endl;
			section &encoded_section = image.add_section(packed_);
			image.set_section_virtual_size(encoded_section, total_virtual_size);
			cout << "Adding unpacker section" << endl;
			section &packer_s = image.add_section(packer_section);
			
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
		cout << "Exception: "<< e.what() << endl;
	}
	getchar();
	return 0;
}