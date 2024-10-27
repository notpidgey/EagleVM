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

    ir_insts calculate_sf(ir_size size)
    {
        ir_insts insts;

        insts.append_range(copy_to_top(size, result));
        insts.append_range(ir_insts{
            // value >> sizeof(value) - 1
            std::make_shared<cmd_push>(static_cast<uint64_t>(size) - 1, size),
            std::make_shared<cmd_shr>(size),

            // value << index_of(ZYDIS_CPUFLAG_SF)
            std::make_shared<cmd_push>(flag_index(ZYDIS_CPUFLAG_SF), size),
            std::make_shared<cmd_shl>(size),

            std::make_shared<cmd_resize>(ir_size::bit_64, size),
            std::make_shared<cmd_or>(ir_size::bit_64),
        });

        return insts;
    }

    ir_insts calculate_zf(ir_size size)
    {
        ir_insts insts;

        insts.append_range(copy_to_top(size, result));
        insts.append_range(ir_insts{
            std::make_shared<cmd_push>(0, size),
            std::make_shared<cmd_cmp>(size),

            std::make_shared<cmd_flags_load>(vm_flags::eq),
            std::make_shared<cmd_push>(flag_index(ZYDIS_CPUFLAG_ZF), size),
            std::make_shared<cmd_shl>(size),

            std::make_shared<cmd_resize>(ir_size::bit_64, size),
            std::make_shared<cmd_or>(ir_size::bit_64),
        });

        return insts;
    }

    ir_insts calculate_pf(ir_size size)
    {
        ir_insts insts;

        insts.append_range(copy_to_top(size, result));
        insts.append_range(ir_insts{
            std::make_shared<cmd_push>(0xFF, size),
            std::make_shared<cmd_and>(size),

            // set PF to 1 only if the number of bits is even
            std::make_shared<cmd_cnt>(size),
            std::make_shared<cmd_push>(1, size),
            std::make_shared<cmd_and>(size),
            std::make_shared<cmd_push>(1, size),
            std::make_shared<cmd_xor>(size),
            std::make_shared<cmd_push>(flag_index(ZYDIS_CPUFLAG_PF), size),
            std::make_shared<cmd_shl>(size),

            std::make_shared<cmd_resize>(ir_size::bit_64, size),
            std::make_shared<cmd_or>(ir_size::bit_64),
        });

        return insts;
    }
}
