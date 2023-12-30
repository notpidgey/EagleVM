#include "pe/pe_generator.h"

void pe_generator::load_existing(std::vector<char>& existing)
{
	//
	// dos header
	///
	
	// copy dos header
	PIMAGE_DOS_HEADER existing_dos_header = (PIMAGE_DOS_HEADER)existing.data();
	memcpy(&dos_header, existing_dos_header, sizeof IMAGE_DOS_HEADER);

	//
	// dos stub
	//
	
	// copy dos stub
	memcpy(&dos_stub, existing.data() + sizeof IMAGE_DOS_HEADER, sizeof dos_stub);

	// remove rich header
	// memset(&dos_stub + 0x80, 0, sizeof dos_stub - 0x80);

	//
	// nt header
	//

	// copy nt header
	PIMAGE_NT_HEADERS existing_nt_header = (PIMAGE_NT_HEADERS)(existing.data() + existing_dos_header->e_lfanew);
	memcpy(&nt_header, existing_nt_header, sizeof IMAGE_NT_HEADERS);

	// signature

	// file header
	auto file = &nt_header.FileHeader;
	file->TimeDateStamp = 0;


	// optional header
	auto optional = &nt_header.OptionalHeader;
	optional->MajorImageVersion = UINT16_MAX;
	optional->MinorImageVersion = UINT16_MAX;
	optional->SizeOfStackCommit += 20 * 8;

	//
	// section headers
	//

	// copy section headers
	PIMAGE_SECTION_HEADER current_section = IMAGE_FIRST_SECTION(existing_nt_header);
	for (int i = 0; i < file->NumberOfSections; i++)
	{
		std::vector<char> current_section_raw(current_section->SizeOfRawData, 0);
		memcpy(current_section_raw.data(), existing.data() + current_section->PointerToRawData, current_section->SizeOfRawData);

		sections.push_back({ *current_section, current_section_raw });
		current_section++;
	}
}

void pe_generator::add_section(const PIMAGE_SECTION_HEADER section_header)
{
	// section_headers.push_back(*section_header);
}

void pe_generator::add_section(const IMAGE_SECTION_HEADER section_header)
{
	// section_headers.push_back(section_header);
}

void pe_generator::save_file(const std::string& save_path)
{
	std::ofstream protected_binary(save_path, std::ios::binary);
	protected_binary.write((char*)&dos_header, sizeof dos_header);
	protected_binary.write((char*)&dos_stub, sizeof dos_stub);
	protected_binary.write((char*)&nt_header, sizeof nt_header);

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

	uint32_t total_written = sizeof dos_header + sizeof dos_stub + sizeof nt_header + total_sections_size;
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

IMAGE_SECTION_HEADER* pe_generator::get_section_rva(const uint32_t rva)
{
	for (auto& [section_header, _] : sections)
	{
		if (section_header.VirtualAddress <= rva && rva < section_header.VirtualAddress + section_header.Misc.VirtualSize)
		{
			return &section_header;
		}
	}

	return nullptr;
}

IMAGE_SECTION_HEADER* pe_generator::get_section_offset(const uint32_t offset)
{
	for (auto& [section_header, _] : sections)
	{
		if (section_header.PointerToRawData <= offset && offset < section_header.PointerToRawData + section_header.SizeOfRawData)
		{
			return &section_header;
		}
	}

	return nullptr;
}

uint32_t pe_generator::offset_to_rva(const uint32_t offset)
{
	const auto section = get_section_offset(offset);
	if (!section)
		return 0;

	return (section->VirtualAddress + offset) - section->PointerToRawData;
}

uint32_t pe_generator::rva_to_offset(const uint32_t rva)
{
	const auto section = get_section_rva(rva);
	if (!section)
		return 0;

	return section->PointerToRawData + rva - section->VirtualAddress;
}