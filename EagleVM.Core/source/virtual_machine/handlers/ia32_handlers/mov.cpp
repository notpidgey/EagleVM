#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/mov.h"

void ia32_mov_handler::construct_single(function_container& container, reg_size size, uint8_t operands, handler_override override)
{
    // value we want to move should be located at the top of the stack
    // the address we want to move TO should be located right below it

    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    auto target_temp = zydis_helper::get_bit_version(VTEMP, size);

    // pop                  ; load the value into VTEMP
    // mov VTEMP2, [VSP]    ; now the address at VSP is the address we want to write to
    // mov [VTEMP2], VTEMP  ; write VTEMP into the address we have
    // pop                  ; we want to maintain the stack
    call_vm_handler(container, pop_handler->get_handler_va(size, 1));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZMEMBD(VSP, 0, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VTEMP2, 0, size), ZREG(target_temp)));
    call_vm_handler(container, pop_handler->get_handler_va(bit64, 1));

    create_vm_return(container);
}

void ia32_mov_handler::finalize_translate_to_virtual(
    const zydis_decode& decoded_instruction, function_container& container)
{
    auto op = decoded_instruction.operands[0];
    if(op.type == ZYDIS_OPERAND_TYPE_REGISTER)
    {
        const zydis_register reg = op.reg.value;
        const reg_size reg_size = zydis_helper::get_reg_size(reg);

        if(reg_size == bit32)
        {
            // clear the upper 32 bits of the operand before mov happens
            const auto [displacement, size] = rm_->get_stack_displacement(reg);

            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VREGS, displacement, 8)));
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VTEMP, 0, 8), ZIMMS(0)));
        }
    }

    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);
}

int ia32_mov_handler::get_op_action(const zydis_decode& inst, zyids_operand_t op_type, int index)
{
    if(index == 0)
        return vm_op_action::action_address;

    return inst_handler_entry::get_op_action(inst, op_type, index);
}
