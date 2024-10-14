#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"

namespace eagle::ir::handler::util
{
    uint8_t flag_index(uint64_t mask)
    {
        unsigned long index = 0;
        _BitScanForward(&index, mask);

        return index;
    }

    ir_insts calculate_sf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& value, bool restore = true)
    {
        ir_insts insts;
        insts.reserve(restore ? 5 : 3);

        if (restore)
            insts.push_back(std::make_shared<cmd_push>(value, size));

        insts.append_range(ir_insts{
            make_dyn(codec::m_shr, encoder::reg(value), encoder::imm((uint64_t)size - 1)),
            make_dyn(codec::m_shl, encoder::reg(value), encoder::imm((flag_index(ZYDIS_CPUFLAG_SF)))),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(value)),
        });

        if (restore)
            insts.push_back(std::make_shared<cmd_pop>(value, size));
    }

    ir_insts calculate_zf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& value, bool restore = true)
    {
        ir_insts insts;
        insts.reserve(restore ? 5 : 3);

        if (restore)
            insts.push_back(std::make_shared<cmd_push>(value, size));

        insts.append_range(ir_insts{
            make_dyn(codec::m_cmp, encoder::reg(value), encoder::imm(0)),
            make_dyn(codec::m_cmovz, encoder::reg(value), encoder::imm(ZYDIS_CPUFLAG_ZF)),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(value)),
        });

        if (restore)
            insts.push_back(std::make_shared<cmd_pop>(value, size));
    }

    ir_insts calculate_pf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& value, bool restore = true)
    {
        ir_insts insts;
        insts.reserve(restore ? 6 : 4);

        if (restore)
            insts.push_back(std::make_shared<cmd_push>(value, size));

        insts.append_range(ir_insts{
            make_dyn(codec::m_and, encoder::reg(value), encoder::imm(0xFF)),
            make_dyn(codec::m_popcnt, encoder::reg(value), encoder::reg(value)),

            make_dyn(codec::m_test, encoder::reg(value), encoder::imm(1)),
            make_dyn(codec::m_xor, encoder::reg(value), encoder::reg(value)),
            make_dyn(codec::m_cmovnz, encoder::reg(value), encoder::imm(ZYDIS_CPUFLAG_PF)),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(value)),
        });

        if (restore)
            insts.push_back(std::make_shared<cmd_pop>(value, size));
    }
}
