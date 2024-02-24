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
