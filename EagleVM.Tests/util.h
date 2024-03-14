#pragma once
#include <Windows.h>
#include <vector>
#include <string>

#include "nlohmann/json.hpp"

enum comparison_fail
{
    none = 0b0,
    rip_mismatch = 0b1,
    flags_mismatch = 0b10,
    register_mismatch = 0b100
};

namespace util
{
    std::vector<uint8_t> parse_hex(const std::string& hex);
    void print_regs(nlohmann::json& inputs, std::ofstream& stream);
    uint64_t* get_value(CONTEXT& new_context, std::string& reg);
};