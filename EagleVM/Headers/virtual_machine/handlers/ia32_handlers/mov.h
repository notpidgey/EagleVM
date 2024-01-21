#pragma once
#include "virtual_machine/handlers/vm_handler_entry.h"

class ia32_mov_handler : public vm_handler_entry
{
public:
    ia32_mov_handler();

private:
    dynamic_instructions_vec construct_single(reg_size size) override;
    bool hook_builder_operand(const zydis_decode& decoded, dynamic_instructions_vec& instructions, int index) override;
};
