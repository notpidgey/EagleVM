#pragma once
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_movsx_handler final : public inst_handler_entry
{
public:
    ia32_movsx_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
        : inst_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 2 },
            { bit32, 2 },
            { bit16, 2 },
        };
    }

private:
    void construct_single(function_container& container, reg_size size, uint8_t operands) override;
    encode_status encode_operand(
        function_container& container, const zydis_decode& instruction, zydis_dreg op_reg, encode_ctx& context) override;
    encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, encode_ctx& context) override;
    bool virtualize_as_address(const zydis_decode& inst, int index) override;

    void upscale_temp(function_container& container, reg_size target, reg_size current);
};