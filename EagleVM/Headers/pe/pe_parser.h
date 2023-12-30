#pragma once
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <functional>

#define SIZE_OF_QWORD_CALL 6

struct pe_import
{
	std::string import_dll;
	std::string import_name;
};

struct pe_protected_section
{
	void* data_begin; //includes begin call
	void* data_end;	  //includes end call
	
	void* instruction_protect_begin;
	void* instruction_protect_end;

	size_t get_instruction_size()
	{
		return (size_t)instruction_protect_end - (size_t)instruction_protect_begin;
	}
};

enum class stub_import
{
	unknown = 0,
	vm_begin,
	vm_end,
};

class pe_parser
{
public:
	std::vector<PIMAGE_SECTION_HEADER> image_sections;
	std::vector<std::pair<pe_import, void*>> image_imports;

	std::vector<char> unprotected_pe_;


	explicit pe_parser(const char* path);
	bool read_file(const char* path);
	int get_file_size();

	std::vector<std::pair<uint32_t, stub_import>> find_iat_calls();

	PIMAGE_DOS_HEADER get_dos_header();
	PIMAGE_NT_HEADERS get_nt_header();
	PIMAGE_SECTION_HEADER get_import_section();
	IMAGE_DATA_DIRECTORY get_directory(short directory);
	
	std::vector<PIMAGE_SECTION_HEADER> get_sections();
	PIMAGE_SECTION_HEADER get_section(const std::string& section_name);
	PIMAGE_SECTION_HEADER get_section_rva(const uint32_t rva);
	PIMAGE_SECTION_HEADER get_section_offset(const uint32_t offset);

	void enum_imports(std::function<void(PIMAGE_IMPORT_DESCRIPTOR, PIMAGE_THUNK_DATA, PIMAGE_SECTION_HEADER, int, char*)> import_proc);

	pe_protected_section offset_to_ptr(uint32_t offset_begin, uint32_t offset_end);

	uint32_t offset_to_rva(uint32_t offset);
	uint32_t rva_to_offset(uint32_t rva);
private:
};