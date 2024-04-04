#pragma once
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"

namespace eagle::virt::handle
{
    class ia32_movsx_handler final : public inst_handler_entry
    {
    public:
        ia32_movsx_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
            : inst_handler_entry(manager, handler_generator)
        {
            handlers = {
                {bit64, 2},
                {bit32, 2},
                {bit16, 2},
            };
        }

    private:
        void construct_single(asmb::function_container& container, reg_size size, uint8_t operands, handler_override override,
            bool inlined = false) override;
        encode_status encode_operand(
            asmb::function_container& container, const zydis_decode& instruction, zydis_dreg op_reg, encode_ctx& context) override;
        encode_status encode_operand(asmb::function_container& container, const zydis_decode& instruction, zydis_dmem op_mem,
            encode_ctx& context) override;

        int get_op_action(const zydis_decode& inst, zyids_operand_t op_type, int index) override;

        void upscale_temp(asmb::function_container& container, reg_size target, reg_size current);
        void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, asmb::function_container& container);
    };
}
