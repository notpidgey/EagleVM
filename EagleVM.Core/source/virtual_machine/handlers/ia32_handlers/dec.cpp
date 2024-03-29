#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/dec.h"

void ia32_dec_handler::construct_single(function_container& container, reg_size reg_size, uint8_t operands, handler_override override)
{
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];

    // pop VTEMP                    ; VTEMP should be an address (always 64 bits)
    call_vm_handler(container, pop_handler->get_handler_va(bit64, 1));

    //dec [VTEMP]                   ; dec popped value
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_DEC, ZMEMBD(VTEMP, 0, reg_size)));

    create_vm_return(container);
}

void ia32_dec_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_LOAD];
    call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));

    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    // accept changes to rflags
    const vm_handler_entry* rflag_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_ACCEPT];
    call_vm_handler(container, rflag_handler->get_vm_handler_va(bit64));
}

int ia32_dec_handler::get_op_action(const zydis_decode& inst, zyids_operand_t op_type, int index)
{
    if (index == 0)
        return vm_op_action::action_address;

    return inst_handler_entry::get_op_action(inst, op_type, index);
}
