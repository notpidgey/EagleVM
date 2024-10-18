#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"

namespace eagle::ir::handler::util
{
    uint8_t flag_index(const uint64_t mask)
    {
        unsigned long index = 0;
        _BitScanForward(&index, mask);

        return index;
    }

     ir_insts calculate_sf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& value, const bool restore)
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

        return insts;
    }

     ir_insts calculate_zf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& result, const bool restore)
    {
        ir_insts insts;
        insts.reserve(restore ? 5 : 3);

        if (restore)
            insts.push_back(std::make_shared<cmd_push>(result, size));

        insts.append_range(ir_insts{
            make_dyn(codec::m_cmp, encoder::reg(result), encoder::imm(0)),
            make_dyn(codec::m_cmovz, encoder::reg(result), encoder::imm(ZYDIS_CPUFLAG_ZF)),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(result)),
        });

        if (restore)
            insts.push_back(std::make_shared<cmd_pop>(result, size));

        return insts;
    }

     ir_insts calculate_pf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& result, const bool restore)
    {
        ir_insts insts;
        insts.reserve(restore ? 6 : 4);

        if (restore)
            insts.push_back(std::make_shared<cmd_push>(result, size));

        insts.append_range(ir_insts{
            make_dyn(codec::m_and, encoder::reg(result), encoder::imm(0xFF)),
            make_dyn(codec::m_popcnt, encoder::reg(result), encoder::reg(result)),

            make_dyn(codec::m_test, encoder::reg(result), encoder::imm(1)),
            make_dyn(codec::m_xor, encoder::reg(result), encoder::reg(result)),
            make_dyn(codec::m_cmovnz, encoder::reg(result), encoder::imm(ZYDIS_CPUFLAG_PF)),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(result)),
        });

        if (restore)
            insts.push_back(std::make_shared<cmd_pop>(result, size));

        return insts;
    }
}
