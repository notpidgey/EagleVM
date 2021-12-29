#pragma once
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <iostream>

class pe_parser
{
public:
	std::vector<PIMAGE_SECTION_HEADER> image_sections;

	explicit pe_parser(const char* path);
	bool read_file(const char* path);
	int get_file_size();

	PIMAGE_DOS_HEADER get_dos_header();
	PIMAGE_NT_HEADERS get_nt_header();
	
	std::vector<PIMAGE_SECTION_HEADER> get_sections();
	PIMAGE_SECTION_HEADER get_import_section();

	std::vector<std::pair<std::string, std::string>> get_dll_imports();
	
private:
	std::vector<char> unprotected_pe_;
};