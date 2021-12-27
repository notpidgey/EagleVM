#pragma once
#include <vector>

class pe_section
{
public:
	virtual std::vector<char>& generate_section() = 0;

private:
	std::vector<char> section_data;
};