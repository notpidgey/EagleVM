#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/cmp.h"

void ia32_cmp_handler::construct_single(function_container& container, reg_size size, uint8_t operands)
{
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];

    // pop and store in VTEMP2
    call_vm_handler(container, pop_handler->get_handler_va(size, 1));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZREG(VTEMP)));

    // pop (automatically stored in VTEMP)
    call_vm_handler(container, pop_handler->get_handler_va(size, 1));

    // compare
    auto target_temp = zydis_helper::get_bit_version(VTEMP, size);
    auto target_temp2 = zydis_helper::get_bit_version(VTEMP2, size);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_CMP, ZREG(target_temp), ZREG(target_temp2)));
        
    create_vm_return(container);
}   

void ia32_cmp_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_LOAD];
    call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));

    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    // accept changes to rflags
    const vm_handler_entry* rflag_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_ACCEPT];
    call_vm_handler(container, rflag_handler->get_vm_handler_va(bit64));
}

encode_status ia32_cmp_handler::encode_operand(
    function_container& container, const zydis_decode& instruction, zydis_dimm op_imm, encode_ctx& context)
{
    auto [stack_disp, orig_rva, index] = context;
    const auto target_size = static_cast<reg_size>(instruction.instruction.operand_width / 8);

    const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(op_imm.value.u)));

    if(instruction.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
        zydis_helper::get_reg_size(instruction.operands[0].reg.value) == bit64)
    {
        // we need to sign extend the IMM value
        upscale_temp(container, bit64, bit32);
    }

    call_vm_handler(container, push_handler->get_handler_va(target_size, 1));

    *stack_disp += target_size;
    return encode_status::success;
}

void ia32_cmp_handler::upscale_temp(function_container& container, reg_size target_size, reg_size current_size)
{
    // mov eax/ax/al, VTEMP
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV,
        ZREG(zydis_helper::get_bit_version(GR_RAX, current_size)),
        ZREG(zydis_helper::get_bit_version(VTEMP, current_size))
    ));

    // keep upgrading the operand until we get to destination size
    while(current_size != target_size)
    {
        // other sizes should not be possible
        switch(current_size)
        {
            case bit32:
            {
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_CDQE));
                current_size = bit64;
                break;
            }
            case bit16:
            {
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_CWDE));
                current_size = bit32;
                break;
            }
            case bit8:
            {
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_CBW));
                current_size = bit16;
                break;
            }
        }
    }

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV,
        ZREG(zydis_helper::get_bit_version(VTEMP, target_size)),
        ZREG(zydis_helper::get_bit_version(GR_RAX, target_size))
    ));
}