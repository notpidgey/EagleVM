#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/cmp.h"

void ia32_cmp_handler::construct_single(function_container& container, reg_size size, uint8_t operands)
{
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_PUSH_RFLAGS];

    // pop and store in VTEMP2
    call_vm_handler(container, pop_handler->get_handler_va(size, 1));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZREG(VTEMP)));

    // pop (automatically stored in VTEMP)
    call_vm_handler(container, pop_handler->get_handler_va(size, 1));

    // compare
    auto target_temp = zydis_helper::get_bit_version(VTEMP, size);
    auto target_temp2 = zydis_helper::get_bit_version(VTEMP2, size);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_CMP, ZREG(target_temp), ZREG(target_temp2)));

    call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));
        
    create_vm_return(container);
}   

void ia32_cmp_handler::finalize_translate_to_virtual(
    const zydis_decode& decoded_instruction, function_container& container)
{
    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    const vm_handler_entry* rflag_handler = hg_->v_handlers[MNEMONIC_VM_POP_RFLAGS];
    call_vm_handler(container, rflag_handler->get_vm_handler_va(bit64));
}