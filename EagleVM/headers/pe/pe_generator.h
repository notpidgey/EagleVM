#pragma once
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <algorithm>
#include <fstream>

#include <Windows.h>

#include "pe/pe_parser.h"
#include "pe/pe_sections/pe_code_section.h"
#include "pe/pe_sections/pe_handler_section.h"

using generator_section_t = std::pair<IMAGE_SECTION_HEADER, std::vector<uint8_t>>;

class pe_generator
{
public:
    IMAGE_DOS_HEADER dos_header;
    IMAGE_NT_HEADERS nt_headers;
    std::vector<generator_section_t> sections;

    explicit pe_generator(pe_parser* pe_parser)
    {
        parser = pe_parser;
        dos_header = {};
        dos_stub = {};
        nt_headers = {};
        sections = {};
    }

    void load_parser();

    generator_section_t& add_section(const char* name);
    void add_section(PIMAGE_SECTION_HEADER section_header);
    void add_section(IMAGE_SECTION_HEADER section_header);

    void add_ignores(const std::vector<std::pair<uint32_t, uint8_t>>& ignore);

    void add_randoms(const std::vector<std::pair<uint32_t, uint8_t>>& random);

    void add_inserts(std::vector<std::pair<uint32_t, std::vector<uint8_t>>>& insert);

    void bake_modifications();

    std::vector<generator_section_t>& get_sections();
    generator_section_t& get_last_section();
    void remove_section(const char* section_name);

    static std::string section_name(const IMAGE_SECTION_HEADER& section);

    void save_file(const std::string& save_path);

    void zero_memory_rva(uint32_t rva, uint32_t size);

    uint32_t align_section(uint32_t value) const;
    uint32_t align_file(uint32_t value) const;

private:
    pe_parser* parser;

    // TODO: probably just make these public? why does it even matter
    std::vector<uint8_t> dos_stub;

    std::vector<std::pair<uint32_t, uint8_t>> va_ignore;
    std::vector<std::pair<uint32_t, uint8_t>> va_random;
    std::vector<std::pair<uint32_t, std::vector<uint8_t>>> va_insert;
};