#pragma once
#include <Windows.h>
#include <vector>
#include <string>

#include "nlohmann/json.hpp"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

enum comparison_fail
{
    none = 0b0,
    rip_mismatch = 0b1,
    flags_mismatch = 0b10,
    register_mismatch = 0b100,
    stack_misalign = 0b1000,
};

namespace test_util
{
    std::vector<uint8_t> parse_hex(const std::string& hex);
    void print_regs(nlohmann::json& inputs, std::stringstream& stream);
    uint64_t* get_value(CONTEXT& new_context, std::string& reg);
};