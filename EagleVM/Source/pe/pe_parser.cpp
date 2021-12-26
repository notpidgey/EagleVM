#include "PE/pe_parser.h"

pe_parser::pe_parser(const char* path)
{
    read_file(path);
}

bool pe_parser::read_file(const char* path)
{
    constexpr int max_filepath = 255;
    char file_name[max_filepath] = {0};
    memcpy_s(&file_name, max_filepath, path, max_filepath);

    DWORD bytes_read = NULL;
    const HANDLE file = CreateFileA(file_name, GENERIC_ALL, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE)
        return false;

    unprotected_pe_.resize(GetFileSize(file, nullptr));
    ReadFile(file, unprotected_pe_.data(), unprotected_pe_.size(), &bytes_read, nullptr);
    CloseHandle(file);

    return false;
}

PIMAGE_DOS_HEADER pe_parser::get_dos_header()
{
    return reinterpret_cast<PIMAGE_DOS_HEADER>(unprotected_pe_.data());
}

PIMAGE_NT_HEADERS pe_parser::get_nt_header()
{
    auto dos_header = get_dos_header();

    return reinterpret_cast<PIMAGE_NT_HEADERS>((char*)dos_header + dos_header->e_lfanew);
}

std::vector<PIMAGE_SECTION_HEADER>& pe_parser::get_sections()
{
    PIMAGE_NT_HEADERS nt_headers = get_nt_header();
    PIMAGE_SECTION_HEADER section_header = IMAGE_FIRST_SECTION(nt_headers);

    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
    {
        image_sections.push_back(section_header);
        section_header++;
    }

    return image_sections;
}

PIMAGE_SECTION_HEADER pe_parser::get_import_section()
{
    const PIMAGE_NT_HEADERS nt_headers = get_nt_header();
    const DWORD import_directory_rva = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    for (const auto& section_header : image_sections)
    {
        if (import_directory_rva >= section_header->VirtualAddress && import_directory_rva < section_header->VirtualAddress + section_header->Misc.VirtualSize)
        {
            return section_header;
        }
    }

    return nullptr;
}

std::vector<std::pair<std::string, std::string>> pe_parser::get_dll_imports()
{
    std::vector<std::pair<std::string, std::string>> image_imports;

    const PIMAGE_NT_HEADERS image_nt_headers = get_nt_header();
    const PIMAGE_SECTION_HEADER import_section = get_import_section();

    const auto raw_offset = unprotected_pe_.data() + import_section->PointerToRawData;

    std::cout << "[>] Retreiving Imports" << std::endl;
    for (PIMAGE_IMPORT_DESCRIPTOR import_descriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
             raw_offset + (image_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - import_section->VirtualAddress)
         ); import_descriptor->Name != 0; import_descriptor++)
    {
        const char* import_library = raw_offset + (import_descriptor->Name - import_section->VirtualAddress);
        std::cout << "\t[>] " << import_library << std::endl;

        auto thunk = import_descriptor->OriginalFirstThunk == 0 ? import_descriptor->FirstThunk : import_descriptor->OriginalFirstThunk;
        for (PIMAGE_THUNK_DATA thunk_data = reinterpret_cast<PIMAGE_THUNK_DATA>(raw_offset + (thunk - import_section->VirtualAddress)); thunk_data->u1.AddressOfData != 0; thunk_data++)
        {
            const char* import_name = raw_offset + (thunk_data->u1.AddressOfData - import_section->VirtualAddress + 2);
            if (thunk_data->u1.AddressOfData < 0x80000000)
            {
                std::cout << "\t\t[+] " << import_name << std::endl;
                image_imports.push_back({ import_library, import_name });
            }
        }
    }

    std::cout << std::endl;
    return image_imports;
}
