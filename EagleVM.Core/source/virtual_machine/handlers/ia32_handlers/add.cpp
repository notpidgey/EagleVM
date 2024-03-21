#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/add.h"

void ia32_add_handler::construct_single(function_container& container, reg_size reg_size, uint8_t operands)
{
    uint64_t size = reg_size;

    const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_PUSH_RFLAGS];
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];

    const zydis_register target_temp = zydis_helper::get_bit_version(VTEMP, reg_size);

    // pop VTEMP
    call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));

    // add size ptr [VSP], VTEMP       ; subtracts topmost value from 2nd top most value
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZMEMBD(VSP, 0, size), ZREG(target_temp)));

    // push rflags                      ; push rflags in case we want to accept these changes
    call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));

    create_vm_return(container);
}

void ia32_add_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    const reg_size target_size = static_cast<reg_size>(decoded_instruction.instruction.operand_width / 8);
    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    // accept changes to rflags
    const vm_handler_entry* rflag_handler = hg_->v_handlers[MNEMONIC_VM_POP_RFLAGS];
    call_vm_handler(container, rflag_handler->get_vm_handler_va(bit64));

    // pop VTEMP2, this will contain the value which we will write
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    call_vm_handler(container, pop_handler->get_handler_va(target_size, 1));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZREG(VTEMP)));

    // pop VTEMP, this will contain the address we will write to
    call_vm_handler(container, pop_handler->get_handler_va(bit64, 1));

    // mov [address], value
    const zydis_register target_temp = zydis_helper::get_bit_version(VTEMP, target_size);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VTEMP, 0, target_size), ZREG(target_temp)));
}

vm_op_action ia32_add_handler::get_virtualize_action(const zydis_decode& inst, int index)
{
    // we do this because add handler only takes values, but we want to write it
    if(index == 0)
        return vm_op_action::action_both_av;

    return inst_handler_entry::get_virtualize_action(inst, index);
}
