#pragma once
#include <vector>
#include <string>
#include <memory>

#include "pe/pe_sections/pe_section.h"

class pe_code_section : public pe_section
{
	pe_code_section(std::vector<char> setion_data);
};

class pe_handler_section : public pe_section
{

};

typedef std::pair<std::string, std::vector<char>> pe_section_entry;
class pe_generator
{
public:
	std::shared_ptr<pe_section_entry>& create_section(const char* section_name);
	void append_data_to_section(std::vector<char>& data);

private:
	//vector of <section name, section data>
	std::vector<std::shared_ptr<pe_section_entry>> sections;
};