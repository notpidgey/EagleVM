#include "pe/pe_parser.h"

pe_parser::pe_parser(const char* path)
    : path_(path)
{
}

bool pe_parser::read_file()
{
    constexpr int MAX_FILEPATH = 255;
    char file_name[MAX_FILEPATH] = {0};
    memcpy_s(&file_name, MAX_FILEPATH, path_.c_str(), MAX_FILEPATH);

    DWORD bytes_read = NULL;
    const HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE)
        return false;

    unprotected_pe_.resize(GetFileSize(file, nullptr));
    auto success = !!ReadFile(file, unprotected_pe_.data(), (DWORD)unprotected_pe_.size(), &bytes_read, nullptr);
    CloseHandle(file);

    return success;
}

uint32_t pe_parser::get_file_size()
{
    return (uint32_t)unprotected_pe_.size();
}

std::vector<std::pair<uint32_t, stub_import>> pe_parser::find_iat_calls()
{
    image_imports.clear();

    const IMAGE_DATA_DIRECTORY import_directory = get_directory(IMAGE_DIRECTORY_ENTRY_IAT);
    const PIMAGE_SECTION_HEADER import_section = get_import_section();

    uint32_t iat_begin = import_section->PointerToRawData - import_directory.VirtualAddress + import_section->VirtualAddress;
    uint32_t iat_end = iat_begin + import_directory.Size;

    PIMAGE_SECTION_HEADER text_section = get_section(".text");

    std::vector<uint32_t> offsets_import_calls;

    // Iterate over the .text section with a step of 2 (since FF 15 is two bytes)
    for (uint32_t i = text_section->PointerToRawData; i <= text_section->PointerToRawData + text_section->SizeOfRawData; i++)
    {
        // Check for FF 15
        if (unprotected_pe_[i] == 0xFF && unprotected_pe_[i + 1] == 0x15)
        {
            offsets_import_calls.push_back(i);
        }
    }

    std::unordered_map<uint32_t, stub_import> stub_dll_imports;
    enum_imports(
        [&stub_dll_imports](const PIMAGE_IMPORT_DESCRIPTOR import_descriptor, const PIMAGE_THUNK_DATA thunk_data, const PIMAGE_SECTION_HEADER import_section, 
            int index, uint8_t* data_base)
        {
            const uint8_t* import_section_raw = data_base + import_section->PointerToRawData;
            const uint8_t* import_library = const_cast<uint8_t*>(import_section_raw + (import_descriptor->Name - import_section->VirtualAddress));

            if (std::strcmp((char*)import_library, "EagleVMStub.dll") == 0)
            {
                // TODO: this needs fixing because sometimes the imports will be in a different order depending on debug/release optimizations
                stub_import import_type;
                switch (index) 
                {
                case 1:
                    import_type = stub_import::vm_end;
                    break;
                case 0:
                    import_type = stub_import::vm_begin;
                    break;
                default:
                    import_type = stub_import::unknown;
                }

                //call rva , import type
                stub_dll_imports[import_descriptor->FirstThunk + (index * 8)] = import_type;
            }
                
        });

    std::vector<std::pair<uint32_t, stub_import>> offsets_to_vm_macros;
    std::ranges::for_each(offsets_import_calls,
        [this, &iat_begin, &iat_end, &stub_dll_imports, &offsets_to_vm_macros](const uint32_t instruction_offset) {
            const auto data_segment = *reinterpret_cast<uint32_t*>(unprotected_pe_.data() + instruction_offset + 2);
            const auto data_rva = offset_to_rva(instruction_offset) + 6 + data_segment;

            if (const auto ds_offset = rva_to_offset(data_rva); iat_begin <= ds_offset && ds_offset < iat_end)
                if(stub_dll_imports.contains(data_rva))
                    offsets_to_vm_macros.push_back({ instruction_offset, stub_dll_imports[data_rva] });
        });

    return offsets_to_vm_macros;
}

uint8_t* pe_parser::get_base()
{
    return (uint8_t*)unprotected_pe_.data();
}

PIMAGE_DOS_HEADER pe_parser::get_dos_header()
{
    return reinterpret_cast<PIMAGE_DOS_HEADER>(unprotected_pe_.data());
}

std::vector<uint8_t> pe_parser::get_dos_stub()
{
    uint8_t* start = (uint8_t*)get_dos_header() + sizeof(IMAGE_DOS_HEADER);
    uint8_t* end = (uint8_t*)get_nt_header();

    std::vector<uint8_t> dos_stub(end - start);
    std::copy(static_cast<uint8_t*>(start), static_cast<uint8_t*>(end), dos_stub.begin());

    return dos_stub;
}

PIMAGE_NT_HEADERS pe_parser::get_nt_header()
{
    const auto dos_header = get_dos_header();

    return reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<char*>(dos_header) + dos_header->e_lfanew);
}

PIMAGE_SECTION_HEADER pe_parser::get_import_section()
{
    const PIMAGE_NT_HEADERS nt_headers = get_nt_header();
    const DWORD import_directory_rva = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    return get_section_rva(import_directory_rva);
}

IMAGE_DATA_DIRECTORY pe_parser::get_directory(short directory)
{
    const PIMAGE_NT_HEADERS nt_headers = get_nt_header();

    return nt_headers->OptionalHeader.DataDirectory[directory];
}

std::vector<PIMAGE_SECTION_HEADER> pe_parser::get_sections()
{
    if (!image_sections.empty())
        return image_sections;

    PIMAGE_NT_HEADERS nt_headers = get_nt_header();
    PIMAGE_SECTION_HEADER section_header = IMAGE_FIRST_SECTION(nt_headers);

    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
    {
        image_sections.push_back(section_header);
        section_header++;
    }

    return image_sections;
}

PIMAGE_SECTION_HEADER pe_parser::get_section(const std::string& section_name)
{
    const auto section = std::ranges::find_if(image_sections,
        [&section_name](const PIMAGE_SECTION_HEADER image_section) {
            return std::strcmp(reinterpret_cast<const char*>(image_section->Name), section_name.c_str()) == 0;
        });

    return *section;
}

PIMAGE_SECTION_HEADER pe_parser::get_section_rva(const uint32_t rva)
{
    for (const auto& section_header : image_sections)
    {
        if (section_header->VirtualAddress <= rva && rva < section_header->VirtualAddress + section_header->Misc.VirtualSize)
        {
            return section_header;
        }
    }

    return nullptr;
}

PIMAGE_SECTION_HEADER pe_parser::get_section_offset(const uint32_t offset)
{
    for (const auto& section_header : image_sections)
    {
        if (section_header->PointerToRawData <= offset && offset < section_header->PointerToRawData + section_header->SizeOfRawData)
        {
            return section_header;
        }
    }

    return nullptr;
}

void pe_parser::enum_imports(const std::function<void(PIMAGE_IMPORT_DESCRIPTOR, PIMAGE_THUNK_DATA, PIMAGE_SECTION_HEADER, int index, uint8_t*)> import_proc)
{
    const PIMAGE_NT_HEADERS image_nt_headers = get_nt_header();
    const PIMAGE_SECTION_HEADER import_section = get_import_section();

    const auto import_data = unprotected_pe_.data() + import_section->PointerToRawData;
    PIMAGE_IMPORT_DESCRIPTOR import_descriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(import_data + (image_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - import_section->VirtualAddress));
    for (; import_descriptor->Name != 0; import_descriptor++)
    {
        int index = 0;
        
        const DWORD thunk = import_descriptor->OriginalFirstThunk == 0 ? import_descriptor->FirstThunk : import_descriptor->OriginalFirstThunk;
        for (PIMAGE_THUNK_DATA thunk_data = reinterpret_cast<PIMAGE_THUNK_DATA>(import_data + (thunk - import_section->VirtualAddress)); thunk_data->u1.AddressOfData != 0; thunk_data++)
        {
            if (thunk_data->u1.AddressOfData < 0x80000000)
            {
                import_proc(import_descriptor, thunk_data, import_section, index, unprotected_pe_.data());
            }
            
            index++;
        }
    }
}

pe_protected_section pe_parser::offset_to_ptr(uint32_t offset_begin, uint32_t offset_end)
{
    pe_protected_section protect
    {
        &unprotected_pe_[offset_begin],
        &unprotected_pe_[offset_end + SIZE_OF_QWORD_CALL],
        &unprotected_pe_[offset_begin + SIZE_OF_QWORD_CALL],
        &unprotected_pe_[offset_end],
    };

    return protect;
}

uint32_t pe_parser::offset_to_rva(const uint32_t offset)
{
    const auto section = get_section_offset(offset);
    if (!section)
        return 0;

    return (section->VirtualAddress + offset) - section->PointerToRawData;
}

uint32_t pe_parser::rva_to_offset(const uint32_t rva)
{
    const auto section = get_section_rva(rva);
    if (!section)
        return 0;

    return section->PointerToRawData + rva - section->VirtualAddress;
}
