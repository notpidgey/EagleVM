#pragma once
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <algorithm>
#include <fstream>

#include <Windows.h>

#include "pe/pe_parser.h"
#include "pe/pe_sections/pe_code_section.h"
#include "pe/pe_sections/pe_handler_section.h"

class pe_generator
{
public:
	explicit pe_generator(pe_parser* pe_parser)
	{
		parser = pe_parser;
		dos_header = {};
		dos_stub = {};
		nt_headers = {};
		sections = {};
	}

	void load_existing();
	
	void add_section(PIMAGE_SECTION_HEADER section_header);
	void add_section(IMAGE_SECTION_HEADER section_header);

	void save_file(const std::string& save_path);

private:
	pe_parser* parser;

	IMAGE_DOS_HEADER dos_header;
	std::array<char, 0xC0> dos_stub;
	IMAGE_NT_HEADERS nt_headers;
	std::vector<std::pair<IMAGE_SECTION_HEADER, std::vector<char>>> sections;

	IMAGE_SECTION_HEADER* get_section_rva(const uint32_t rva);
	IMAGE_SECTION_HEADER* get_section_offset(const uint32_t offset);

	uint32_t offset_to_rva(uint32_t offset);
	uint32_t rva_to_offset(uint32_t rva);
};