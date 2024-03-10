#include "disassembler/models/basic_block.h"

basic_block::basic_block()
{
    start_rva = 0;
    end_rva_inc = 0;
}

bool basic_block::is_conditional_jump() const
{
    const auto& [instruction, _] = instructions.back();
    return instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && instruction.mnemonic != ZYDIS_MNEMONIC_JMP;
}

bool basic_block::is_jump() const
{
    const auto& [instruction, _] = instructions.back();
    return instruction.meta.branch_type == ZYDIS_MNEMONIC_JMP;
}

bool basic_block::is_final_block() const
{
    return end_reason == block_end;
}

uint64_t basic_block::calc_jump_address(const uint8_t instruction_index)
{
    uint64_t current_rva = 0;
    for(int i = 0; i < instruction_index; i++)
    {
        const auto& [instruction, operands] = instructions[i];
        current_rva += instruction.length;
    }

    const auto& [instruction, operands] = instructions[instruction_index];

    uint64_t target_address;
    ZydisCalcAbsoluteAddress(&instruction, &operands[0], current_rva, &target_address);

    return target_address;
}
