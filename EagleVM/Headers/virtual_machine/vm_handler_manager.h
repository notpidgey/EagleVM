#pragma once
#include <unordered_map>

typedef std::pair<uint32_t, uint32_t> offset_rva_pair;

class vm_handler_manager
{
public:

private:
    std::unordered_map<uint32_t, offset_rva_pair> vm_handler_location_info_;
};
