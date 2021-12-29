#include <algorithm>

#include "pe/pe_parser.h"
#include "pe/pe_generator.h"
#include "virtual_machine/handle_generator.h"

int main(int argc, char* argv[])
{
	pe_parser parser = pe_parser("D:\\VM\\ovtest.exe");
	std::printf("[+] loaded ovtest.exe -> %i bytes\n", parser.get_file_size());

	int i = 1;

	auto sections = parser.get_sections();
	std::printf("[>] parsed %i sections\n", parser.get_sections().size());
	std::printf("%3s %-10s %-10s %-10s\n", "", "name", "va", "size");
	std::for_each(sections.begin(), sections.end(), 
		[&i](PIMAGE_SECTION_HEADER image_section)
		{
			std::printf("%3i %-10s %-10d %-10d\n", i, image_section->Name, image_section->VirtualAddress, image_section->SizeOfRawData);
			i++;
		});

	i = 1;
	auto imports = parser.get_dll_imports();
	std::printf("\n[>] %i imports\n", imports.size());
	std::printf("%3s %-20s %-s\n", "", "source", "import");
	std::for_each(imports.begin(), imports.end(), 
		[&i](std::pair<std::string, std::string>& dll_import)
		{
			std::printf("%3i %-20s %-s\n", i, dll_import.first.c_str(), dll_import.second.c_str());
			i++;
		});

	std::printf("\n[>] porting original pe properties to new executable\n");

	auto nt_header = parser.get_nt_header();
	std::printf("[>] image base -> %I64d bytes\n", nt_header->OptionalHeader.ImageBase);
	std::printf("[>] section alignment -> %i bytes\n", nt_header->OptionalHeader.SectionAlignment);
	std::printf("[>] file alignment -> %i bytes\n", nt_header->OptionalHeader.FileAlignment);
	std::printf("[>] stack reserve -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfStackReserve);
	std::printf("[>] stack commit -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfStackCommit);
	std::printf("[>] heap reserve -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfHeapReserve);
	std::printf("[>] heap commit -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfHeapCommit);

	pe_generator generator;
	PIMAGE_DOS_HEADER dos_header = generator.build_dos_header();
	PIMAGE_FILE_HEADER file_header = generator.build_coff_header();
	PIMAGE_OPTIONAL_HEADER optional_header = generator.build_optional_header(
		nt_header->OptionalHeader.ImageBase,
		nt_header->OptionalHeader.SectionAlignment,
		nt_header->OptionalHeader.FileAlignment,
		nt_header->OptionalHeader.SizeOfStackReserve,
		nt_header->OptionalHeader.SizeOfStackCommit,
		nt_header->OptionalHeader.SizeOfHeapReserve,
		nt_header->OptionalHeader.SizeOfHeapCommit
	);

	//Add already exisiting sections.
	std::for_each(sections.begin(), sections.end(), 
		[&generator](PIMAGE_SECTION_HEADER section_header)
		{
			generator.add_section(section_header);
		});


	vm_handle_generator::create_vm_enter();
	vm_handle_generator::create_vm_exit();

	return 0;
}
