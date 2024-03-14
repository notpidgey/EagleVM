#pragma once
#include <vector>
#include <string>

#include "nlohmann/json.hpp"

namespace util
{
    std::vector<uint8_t> parse_hex(const std::string& hex);
    void print_regs(nlohmann::json& inputs);
};