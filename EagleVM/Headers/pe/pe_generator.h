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

#define P2ALIGNUP(x, align) (-(-((LONG64)x) & -((LONG64)align)))

typedef std::pair<IMAGE_SECTION_HEADER, std::vector<char>> generator_section_t;

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

	void load_parser();
	
	PIMAGE_SECTION_HEADER add_section();
	void add_section(PIMAGE_SECTION_HEADER section_header);
	void add_section(IMAGE_SECTION_HEADER section_header);

	std::vector<generator_section_t> get_sections();
	generator_section_t& get_last_section();

	void save_file(const std::string& save_path);

private:
	pe_parser* parser;

	IMAGE_DOS_HEADER dos_header;
	std::array<char, 0xC0> dos_stub;
	IMAGE_NT_HEADERS nt_headers;
	std::vector<generator_section_t> sections;
};