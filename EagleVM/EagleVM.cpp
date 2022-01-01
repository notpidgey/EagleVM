#include <algorithm>

#include "pe/pe_parser.h"
#include "pe/pe_generator.h"
#include "virtual_machine/handle_generator.h"

int main(int argc, char* argv[])
{
	pe_parser parser = pe_parser("D:\\VM\\EagleVMSandbox.exe");
	std::printf("[+] loaded ovtest.exe -> %i bytes\n", parser.get_file_size());

	int i = 1;

	std::vector<PIMAGE_SECTION_HEADER> sections = parser.get_sections();
	
	std::printf("[>] parsed %i sections\n", parser.get_sections().size());
	std::printf("%3s %-10s %-10s %-10s\n", "", "name", "va", "size");
	std::ranges::for_each(sections.begin(), sections.end(),
		[&i](const PIMAGE_SECTION_HEADER image_section)
		{
			std::printf("%3i %-10s %-10d %-10d\n", i, image_section->Name, image_section->VirtualAddress, image_section->SizeOfRawData);
			i++;
		});

	i = 1;
	
	std::printf("\n[>] image imports\n");
	std::printf("%3s %-20s %-s\n", "", "source", "import");
	parser.enum_imports(
		[&i](const PIMAGE_IMPORT_DESCRIPTOR import_descriptor, const PIMAGE_THUNK_DATA thunk_data, const PIMAGE_SECTION_HEADER import_section, int, char* data_base) 
		{
			const char* import_section_raw = data_base + import_section->PointerToRawData;
			const char* import_library = const_cast<char*>(import_section_raw + (import_descriptor->Name - import_section->VirtualAddress));
			const char* import_name = const_cast<char*>(import_section_raw + (thunk_data->u1.AddressOfData - import_section->VirtualAddress + 2));

			std::printf("%3i %-20s %-s\n", i, import_library, import_name);
			i++;
		});

	std::printf("\n[+] porting original pe properties to new executable...\n");

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
	std::ranges::for_each(sections, 
		[&generator](const PIMAGE_SECTION_HEADER section_header)
		{
			generator.add_section(section_header);
		});

	std::printf("\n[+] searching for uses of vm macros...\n");

	i = 1;
	std::vector<std::pair<uint32_t, stub_import>> vm_iat_calls = parser.find_iat_calls();
	std::printf("%3s %-10s %-10s\n", "", "rva", "vm");
	std::ranges::for_each(vm_iat_calls.begin(), vm_iat_calls.end(), 
		[&i, &parser](const auto call)
		{
			auto [file_offset, stub_import] = call;
			switch (stub_import)
			{
			case stub_import::vm_begin:
				std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "vm_begin");
				break;
			case stub_import::vm_end:
				std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "vm_end");
				break;
			case stub_import::unknown:
				std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "?");
				break;
			}

			i++;
		});

	std::printf("\n");

	bool success = true;
	stub_import previous_call = stub_import::vm_end;
	std::ranges::for_each(vm_iat_calls.begin(), vm_iat_calls.end(), 
		[&previous_call, &success](const auto call) {
			if (call.second == previous_call)
			{
				success = false;
				return false;
			}

			previous_call = call.second;
		});

	//vm macros should be in "begin, end, begin, end" order
	//this is not an entirely reliable way to check because it could mean function A has vm_begin and function B will have vm_end
	//meaning it will virtualize in between the two functions, which should never happen.
	//blame the user
	if(success)
		std::printf("[+] successfully verified macro usage\n");
	else
		std::printf("[+] failed to verify macro usage\n");

	vm_handle_generator::create_vm_enter();
	vm_handle_generator::create_vm_exit();

	return 0;
}
