#pragma once
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <algorithm>
#include <fstream>

#include <Windows.h>

#include "pe/pe_sections/pe_code_section.h"
#include "pe/pe_sections/pe_handler_section.h"

#define P2ALIGNUP(x, align) (-(-((LONG64)x) & -((LONG64)align)))

class pe_generator
{
public:
	void load_existing(std::vector<char>& existing);
	
	void add_section(PIMAGE_SECTION_HEADER section_header);
	void add_section(IMAGE_SECTION_HEADER section_header);

	void save_file(const std::string& save_path);

private:
	IMAGE_DOS_HEADER dos_header;
	std::array<char, 0xC0> dos_stub;
	IMAGE_NT_HEADERS nt_header;
	std::vector<std::pair<IMAGE_SECTION_HEADER, std::vector<char>>> sections;

	std::vector<std::pair<IMAGE_IMPORT_DESCRIPTOR, std::vector<IMAGE_THUNK_DATA>>> imports;

	IMAGE_SECTION_HEADER* get_section_rva(const uint32_t rva);
	IMAGE_SECTION_HEADER* get_section_offset(const uint32_t offset);

	uint32_t offset_to_rva(uint32_t offset);
	uint32_t rva_to_offset(uint32_t rva);
};