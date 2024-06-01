#pragma once
#include <vector>

#include "eaglevm-core/codec/zydis_defs.h"

namespace eagle::virt::eg
{
    using reg_range = std::pair<uint16_t, uint16_t>;
    struct reg_map_range
    {
        reg_range source_range;
        std::vector<codec::reg, reg_range> dest_range;
    };

    class inst_regs
    {
    public:
        inst_regs();
        void create_mappings();

    private:
        std::unordered_map<codec::reg, reg_map_range> source_register_map;
        std::vector<std::pair<codec::reg, std::vector<reg_range>>> dest_register_map;
    };
}
