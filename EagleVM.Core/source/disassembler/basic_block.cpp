#include "eaglevm-core/disassembler/basic_block.h"

namespace eagle::dasm
{
    basic_block::basic_block()
    {
        start_rva = 0;
        end_rva_inc = 0;
    }

    block_end_reason basic_block::get_end_reason() const
    {
        const auto& [inst, _] = decoded_insts.back();
        if (inst.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && inst.mnemonic != ZYDIS_MNEMONIC_CALL)
        {
            // this is either JMP or conditional JMP
            if (inst.mnemonic == ZYDIS_MNEMONIC_JMP)
                return block_jump;

            if (inst.mnemonic == ZYDIS_MNEMONIC_RET)
                return block_ret;

            return block_conditional_jump;
        }

        // this could either be a call or regular block end
        return block_end;
    }

    bool basic_block::is_conditional_jump() const
    {
        const auto& [instruction, _] = decoded_insts.back();
        return instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && instruction.mnemonic != ZYDIS_MNEMONIC_JMP;
    }

    bool basic_block::is_jump() const
    {
        const auto& [instruction, _] = decoded_insts.back();
        return instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && instruction.mnemonic == ZYDIS_MNEMONIC_JMP;
    }

    uint64_t basic_block::get_index_rva(const uint32_t index) const
    {
        uint64_t current_rva = start_rva;
        for (auto i = 0; i < index; i++)
        {
            const auto& [instruction, operands] = decoded_insts[i];
            current_rva += instruction.length;
        }

        return current_rva;
    }

    uint64_t basic_block::calc_jump_address(const uint32_t index) const
    {
        const uint64_t current_rva = get_index_rva(index);
        const auto& [instruction, operands] = decoded_insts[index];

        uint64_t target_address;
        ZydisCalcAbsoluteAddress(&instruction, &operands[0], current_rva, &target_address);

        return target_address;
    }
}
