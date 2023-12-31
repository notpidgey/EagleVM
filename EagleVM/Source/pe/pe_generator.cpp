#include "pe/pe_generator.h"

void pe_generator::load_parser()
{
	//
	// dos header
	///
	
	// copy dos header
	PIMAGE_DOS_HEADER existing_dos_header = parser->get_dos_header();
	memcpy(&dos_header, existing_dos_header, sizeof IMAGE_DOS_HEADER);

	//
	// dos stub
	//
	
	// copy dos stub
	std::vector<uint8_t> existing_dos_stub = parser->get_dos_stub();
	memcpy(&dos_stub, existing_dos_stub.data() + sizeof IMAGE_DOS_HEADER, existing_dos_stub.size());

	// remove rich header
	// memset(&dos_stub + 0x80, 0, sizeof dos_stub - 0x80);

	//
	// nt header
	//

	// copy nt header
	PIMAGE_NT_HEADERS existing_nt_header = parser->get_nt_header();
	memcpy(&nt_headers, existing_nt_header, sizeof IMAGE_NT_HEADERS);

	// signature

	// file header
	IMAGE_FILE_HEADER* file = &nt_headers.FileHeader;
	file->TimeDateStamp = 0;

	// optional header
	IMAGE_OPTIONAL_HEADER* optional = &nt_headers.OptionalHeader;
	optional->MajorImageVersion = UINT16_MAX;
	optional->MinorImageVersion = UINT16_MAX;
	optional->SizeOfStackCommit += 20 * 8;

	//
	// section headers
	//

	// copy section headers
	std::vector<PIMAGE_SECTION_HEADER> existing_sections = parser->get_sections();
	for (PIMAGE_SECTION_HEADER section : existing_sections)
	{
		std::vector<char> current_section_raw(section->SizeOfRawData, 0);
		memcpy(current_section_raw.data(), parser->get_base() + section->PointerToRawData, section->SizeOfRawData);

		sections.push_back({ *section, current_section_raw });
	}
}

generator_section_t& pe_generator::add_section(const char* name)
{
	generator_section_t new_section = {};
	strcpy((char*)&(std::get<0>(new_section).Name), name);
	
	sections.push_back(new_section);

	return new_section;
}

void pe_generator::add_section(const PIMAGE_SECTION_HEADER section_header)
{
	// section_headers.push_back(*section_header);
}

void pe_generator::add_section(const IMAGE_SECTION_HEADER section_header)
{
	// section_headers.push_back(section_header);
}

std::vector<generator_section_t> pe_generator::get_sections()
{
	return sections;
}

generator_section_t& pe_generator::get_last_section()
{
	return sections.back();
}

void pe_generator::save_file(const std::string& save_path)
{
	std::ofstream protected_binary(save_path, std::ios::binary);
	protected_binary.write((char*)&dos_header, sizeof IMAGE_DOS_HEADER);
	protected_binary.write((char*)&dos_stub, sizeof dos_stub);
	protected_binary.write((char*)&nt_headers, sizeof IMAGE_NT_HEADERS);

	uint16_t total_sections_size = 0;
	for (auto& [section, _] : sections)
	{
		total_sections_size += sizeof IMAGE_SECTION_HEADER;
		protected_binary.write((char*)&section, sizeof IMAGE_SECTION_HEADER);
	}

	std::sort(sections.begin(), sections.end(), [](auto& a, auto& b)
		{
			auto a_section = std::get<0>(a);
			auto b_section = std::get<0>(b);

			return a_section.PointerToRawData < b_section.PointerToRawData;
		});

	uint32_t total_written = sizeof dos_header + sizeof dos_stub + sizeof nt_headers + total_sections_size;
	for (auto& [section, section_raw] : sections)
	{
		if (total_written != section.PointerToRawData)
		{
			int32_t missing_bytes = section.PointerToRawData - total_written;
			if (missing_bytes < 0)
			{
				// TODO: handle error, this should not be less than 0
				return;
			}

			protected_binary.seekp(missing_bytes, std::ios::cur);
			total_written += missing_bytes;
		}

		protected_binary.write(section_raw.data(), section_raw.size());
		total_written += section_raw.size();
	}
}