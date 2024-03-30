#pragma once
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <functional>

#include <eaglevm-core/pe/models/stub.h>

namespace eagle::pe
{
    class pe_parser
    {
    public:
        std::vector<PIMAGE_SECTION_HEADER> image_sections;

        explicit pe_parser(const char* path);

        bool read_file();
        uint32_t get_file_size();

        std::vector<std::pair<uint32_t, stub_import>> find_iat_calls();

        uint8_t* get_base();

        PIMAGE_DOS_HEADER get_dos_header();
        std::vector<uint8_t> get_dos_stub();
        PIMAGE_NT_HEADERS get_nt_header();
        PIMAGE_SECTION_HEADER get_import_section();
        IMAGE_DATA_DIRECTORY get_directory(short directory);

        std::vector<PIMAGE_SECTION_HEADER> get_sections();
        PIMAGE_SECTION_HEADER get_section(const std::string& section_name);
        PIMAGE_SECTION_HEADER get_section_rva(const uint32_t rva);
        PIMAGE_SECTION_HEADER get_section_offset(const uint32_t offset);

        void enum_imports(std::function<void(PIMAGE_IMPORT_DESCRIPTOR, PIMAGE_THUNK_DATA, PIMAGE_SECTION_HEADER, int, uint8_t*)> import_proc);

        uint8_t* offset_to_ptr(uint32_t offset_begin);
        uint32_t offset_to_rva(uint32_t offset);
        uint32_t rva_to_offset(uint32_t rva);
        uint8_t* rva_to_pointer(uint32_t rva);

    private:
        std::string path_;
        std::vector<uint8_t> unprotected_pe_;
    };
}