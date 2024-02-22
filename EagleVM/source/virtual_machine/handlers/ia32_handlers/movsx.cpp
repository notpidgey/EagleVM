#include "virtual_machine/handlers/ia32_handlers/movsx.h"

#include "virtual_machine/vm_generator.h"

void ia32_movsx_handler::construct_single(function_container& container, reg_size size, uint8_t operands)
{
    const inst_handler_entry* mov_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_MOV];

    // we can literally call the mov handler because we upgraded the operand-
    call_vm_handler(container, mov_handler->get_handler_va(size, 2));

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(size), __func__, container.size());
}

encode_status ia32_movsx_handler::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dreg op_reg, int& stack_disp, int index)
{
    if(index == 0)
    {
        const auto [displacement, size] = rm_->get_stack_displacement(TO64(op_reg.value));
        const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];

        // this means we want to put the address of of the target register at the top of the stack
        // mov VTEMP, VREGS + DISPLACEMENT
        // push

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VREGS, displacement, 8)));
        call_vm_handler(container, push_handler->get_handler_va(bit64, 1)); // always 64 bit because its an address
    }
    else
    {
        const auto [displacement, size] = rm_->get_stack_displacement(op_reg.value);
        const vm_handler_entry* load_handler = hg_->v_handlers[MNEMONIC_VM_LOAD_REG];
        const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
        const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];

        // this routine will load the register value to the top of the VSTACK
        // mov VTEMP, -8
        // VM_LOAD_REG
        // pop VTEMP

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMS(displacement)));
        call_vm_handler(container, load_handler->get_vm_handler_va(size));
        call_vm_handler(container, pop_handler->get_handler_va(size, 1));

        const reg_size current_size = size;
        const reg_size target_size = static_cast<reg_size>(instruction.instruction.operand_width / 8);
        upscale_temp(container, target_size, current_size);

        call_vm_handler(container, push_handler->get_handler_va(target_size, 1));
    }

    return encode_status::success;
}

encode_status ia32_movsx_handler::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, int& stack_disp, int index)
{
    if(op_mem.type != ZYDIS_MEMOP_TYPE_MEM)
        return encode_status::unsupported;

    const vm_handler_entry* lreg_handler = hg_->v_handlers[MNEMONIC_VM_LOAD_REG];
    const auto lreg_address = lreg_handler->get_vm_handler_va(bit64);

    const vm_handler_entry* trash_handler = hg_->v_handlers[MNEMONIC_VM_TRASH_RFLAGS];
    const auto trash_address = trash_handler->get_vm_handler_va(bit64);

    const inst_handler_entry* mul_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_IMUL];
    const auto mul_address = mul_handler->get_handler_va(bit64, 2);

    const inst_handler_entry* add_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_ADD];
    const auto add_address = add_handler->get_handler_va(bit64, 2);

    const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto push_address = push_handler->get_handler_va(bit64, 1);

    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    const auto pop_address = pop_handler->get_handler_va(bit64, 1);

    //[base + index * scale + disp]

    //1. begin with loading the base register
    //mov VTEMP, imm
    //jmp VM_LOAD_REG
    {
        if(op_mem.base != ZYDIS_REGISTER_RSP)
        {
            const auto [base_displacement, base_size] = rm_->get_stack_displacement(op_mem.base);

            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, lreg_address);
        }
        else
        {
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZREG(VSP)));
            if(stack_disp)
            {
                // this is meant to account for any possible pushes we set up
                // if we now access VSP, its not going to be what it was before we called this function
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VSP, stack_disp, 8)));
            }

            call_vm_handler(container, push_address);
        }
    }

    //2. load the index register and multiply by scale
    //mov VTEMP, imm    ;
    //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
    if(op_mem.index != ZYDIS_REGISTER_NONE)
    {
        const auto [index_displacement, index_size] = rm_->get_stack_displacement(op_mem.index);

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMS(index_displacement)));
        call_vm_handler(container, lreg_address);
    }

    if(op_mem.scale != 0)
    {
        //mov VTEMP, imm    ;
        //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
        //jmp VM_MUL        ; multiply INDEX * SCALE
        //vmscratch         ; ignore the rflags we just modified
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(op_mem.scale)));
        call_vm_handler(container, push_address);
        call_vm_handler(container, mul_address);
        call_vm_handler(container, trash_address);
    }

    if(op_mem.index != ZYDIS_REGISTER_NONE)
    {
        call_vm_handler(container, add_address);
        call_vm_handler(container, trash_address);
    }

    if(op_mem.disp.has_displacement)
    {
        // 3. load the displacement and add
        // we can do this with some trickery using LEA so we dont modify rflags

        // pop current value into VTEMP
        // lea VTEMP, [VTEMP +- imm]
        // push

        call_vm_handler(container, pop_address);
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VTEMP, op_mem.disp.value, 8)));
        call_vm_handler(container, push_address);
    }

    const reg_size target_size = reg_size(instruction.instruction.operand_width / 8);
    const reg_size mem_size = reg_size(instruction.operands[index].size / 8);

    // for movsx this is always going to be the second operand
    // this means that we want to get the value and pop it
    call_vm_handler(container, pop_address);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV,
        ZREG(zydis_helper::get_bit_version(VTEMP, mem_size)),
        ZMEMBD(VTEMP, 0, mem_size)
    ));
    upscale_temp(container, target_size, mem_size);
    call_vm_handler(container, push_handler->get_handler_va(target_size, 1));

    return encode_status::success;
}

void ia32_movsx_handler::upscale_temp(function_container& container, reg_size target_size, reg_size current_size)
{
    const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto push_address = push_handler->get_handler_va(bit64, 1);

    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    const auto pop_address = pop_handler->get_handler_va(bit64, 1);

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZREG(GR_RAX)));
    call_vm_handler(container, push_address);

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

    call_vm_handler(container, pop_address);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(GR_RAX), ZREG(VTEMP)));
}
