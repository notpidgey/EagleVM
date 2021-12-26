#include "pe/pe_generator.h"

std::shared_ptr<pe_section_entry>& pe_generator::create_section(const char* section_name)
{
	std::shared_ptr<pe_section_entry> entry = std::make_shared<pe_section_entry>();
	sections.push_back(entry);

	return entry;
}
