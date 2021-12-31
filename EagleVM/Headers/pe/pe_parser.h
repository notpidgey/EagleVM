#pragma once
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <functional>

struct pe_import
{
	std::string import_dll;
	std::string import_name;
};

class pe_parser
{
public:
	std::vector<PIMAGE_SECTION_HEADER> image_sections;
	std::vector<std::pair<pe_import, void*>> image_imports;

	explicit pe_parser(const char* path);
	bool read_file(const char* path);
	int get_file_size();

	std::vector<std::pair<pe_import, void*>>* find_iat_calls();

	PIMAGE_DOS_HEADER get_dos_header();
	PIMAGE_NT_HEADERS get_nt_header();
	
	std::vector<PIMAGE_SECTION_HEADER> get_sections();
	PIMAGE_SECTION_HEADER get_section(const std::string& section_name);
	PIMAGE_SECTION_HEADER get_section_rva(const uint32_t rva);
	PIMAGE_SECTION_HEADER get_section_offset(const uint32_t offset);
	PIMAGE_SECTION_HEADER get_import_section();

	IMAGE_DATA_DIRECTORY get_directory(short directory);

	void enum_imports(std::function<void(PIMAGE_IMPORT_DESCRIPTOR, PIMAGE_THUNK_DATA, PIMAGE_SECTION_HEADER, int index, char*)> import_proc);
private:
	std::vector<char> unprotected_pe_;

	uint32_t offset_to_rva(uint32_t offset);
	uint32_t rva_to_offset(uint32_t rva);
};