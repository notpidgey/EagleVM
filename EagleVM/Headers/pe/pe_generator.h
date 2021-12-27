#pragma once
#include <vector>
#include <string>
#include <memory>
#include <Windows.h>

#include "pe/pe_sections/pe_code_section.h"
#include "pe/pe_sections/pe_handler_section.h"

constexpr auto default_image_base = 140000000;
constexpr auto default_section_alignment = 1000;
constexpr auto default_file_alignment = 200;

class pe_generator
{
public:
	std::vector<char>& build_dos_header();
	std::vector<char>& build_coff_header();
	std::vector<char>& build_optional_header();
	std::vector<char>& build_section_table();
};