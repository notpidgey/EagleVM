#pragma once
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"

namespace eagle::virt::handle
{
    class ia32_cmp_handler : public inst_handler_entry
    {
    public:
        ia32_cmp_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
            : inst_handler_entry(manager, handler_generator)
        {
            handlers = {
                {bit64, 2},
                {bit32, 2},
                {bit16, 2},
                {bit8, 2}
            };
        };

    private:
        void construct_single(asmb::function_container& container, reg_size size, uint8_t operands, handler_override override,
            bool inlined = false) override;
        void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, asmb::function_container& container) override;
        encode_status encode_operand(asmb::function_container& container, const zydis_decode& instruction, zydis_dimm op_imm,
            encode_ctx& context) override;

        void upscale_temp(asmb::function_container& container, reg_size target_size, reg_size current_size);
    };
}
