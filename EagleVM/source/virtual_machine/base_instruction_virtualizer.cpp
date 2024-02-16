#include "virtual_machine/base_instruction_virtualizer.h"

#include "virtual_machine/handlers/vm_handler_generator.h"
#include "virtual_machine/vm_register_manager.h"


base_instruction_virtualizer::base_instruction_virtualizer(vm_register_manager* manager, vm_handler_generator* handler_generator)
{
    rm_ = manager;
    hg_ = handler_generator;
}

std::pair<bool, function_container> base_instruction_virtualizer::translate_to_virtual(
    const zydis_decode& decoded_instruction)
{
    function_container container = {};

    //virtualizer does not support more than 2 operands OR all mnemonics
    if(decoded_instruction.instruction.operand_count > 2 || !hg_->vm_handlers.contains(
        decoded_instruction.instruction.mnemonic))
    {
    INSTRUCTION_NOT_SUPPORTED:
        return { false, container };
    }

    int current_disp = 0;
    for(int i = 0; i < decoded_instruction.instruction.operand_count; i++)
    {
        encode_status status = encode_status::unsupported;
        switch(const zydis_decoded_operand& operand = decoded_instruction.operands[i]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                status = encode_operand(container, decoded_instruction, operand.reg, current_disp, i);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                status = encode_operand(container, decoded_instruction, operand.mem, current_disp, i);
                break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                status = encode_operand(container, decoded_instruction, operand.ptr, current_disp);
                break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                status = encode_operand(container, decoded_instruction, operand.imm, current_disp);
                break;
        }

        if(status != encode_status::success)
            goto INSTRUCTION_NOT_SUPPORTED;
    }

    finalize_translate_to_virtual(decoded_instruction, container);
    return { true, container };
}

void base_instruction_virtualizer::create_vm_return(function_container& container)
{
    // now that we use a virtual call stack we must pop the top address

    // mov VCSRET, [VCS]        ; pop from call stack
    // lea VCS, [VCS + 8]       ; move up the call stack pointer
    // lea VIP, [0x14000000]    ; load base
    // lea VIP, [VIP + VCSRET]  ; add rva to base
    // jmp VIP

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCSRET), ZMEMBD(VCS, 0, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, 8, 8)));

    code_label* rel_label = code_label::create();
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBI(VIP, VCSRET, 1, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));
}

void base_instruction_virtualizer::call_vm_handler(function_container& container, code_label* jump_label)
{
    if(!jump_label)
        __debugbreak(); // jump_label should never be null

    code_label* retun_label = code_label::create();
    code_label* rel_label = code_label::create();

    // lea VCS, [VCS - 8]       ; allocate space for new return address
    // mov [VCS], code_label    ; place return rva on the stack
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VCS, 0, 8), ZLABEL(retun_label))));

    // lea VIP, [0x14000000]    ; load base
    // lea VIP, [VIP + VCSRET]  ; add rva to base
    // jmp VIP
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(VIP, jump_label->get(), 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));

    // execution after VM handler should end up here
    container.assign_label(retun_label);
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction,
                                                           zydis_dreg op_reg, int& stack_disp, int index)
{
    if(first_operand_as_ea == true && index == 0)
    {
        const auto [displacement, size] = rm_->get_stack_displacement(TO64(op_reg.value));
        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];

        // this means we want to put the address of of the target register at the top of the stack
        // mov VTEMP, VREGS + DISPLACEMENT
        // push

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VREGS, displacement, 8)));
        call_vm_handler(container, push_handler->get_handler_va(bit64)); // always 64 bit because its an address
    }
    else
    {
        const auto [displacement, size] = rm_->get_stack_displacement(op_reg.value);
        const vm_handler_entry* load_handler = hg_->vm_handlers[MNEMONIC_VM_LOAD_REG];

        // this routine will load the register value to the top of the VSTACK
        // mov VTEMP, -8
        // call VM_LOAD_REG

        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMS(displacement)));
        call_vm_handler(container, load_handler->get_handler_va(zydis_helper::get_reg_size(op_reg.value)));
    }

    stack_disp += bit64;
    return encode_status::success;
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, int& stack_disp, int index)
{
    //most definitely riddled with bugs
    if(op_mem.type != ZYDIS_MEMOP_TYPE_MEM)
        return encode_status::unsupported;

    const vm_handler_entry* lreg_handler = hg_->vm_handlers[MNEMONIC_VM_LOAD_REG];
    const auto lreg_address = lreg_handler->get_handler_va(bit64);

    const vm_handler_entry* trash_handler = hg_->vm_handlers[MNEMONIC_VM_TRASH_RFLAGS];
    const auto trash_address = trash_handler->get_handler_va(bit64);

    const vm_handler_entry* mul_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_MUL];
    const auto mul_address = mul_handler->get_handler_va(bit64);

    const vm_handler_entry* add_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_ADD];
    const auto add_address = add_handler->get_handler_va(bit64);

    const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto push_address = push_handler->get_handler_va(bit64);

    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    const auto pop_address = pop_handler->get_handler_va(bit64);

    //[base + index * scale + disp]

    //1. begin with loading the base register
    //mov VTEMP, imm
    //jmp VM_LOAD_REG
    {
        if(op_mem.base != ZYDIS_REGISTER_RSP)
        {
            const auto [base_displacement, base_size] = rm_->get_stack_displacement(op_mem.base);

            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, lreg_address);
        }
        else
        {
            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VTEMP), ZREG(VSP)));
            if(stack_disp)
            {
                // this is meant to account for any possible pushes we set up
                // if we now access VSP, its not going to be what it was before we called this function
                container.add(zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, stack_disp, 8)));
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

        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMS(index_displacement)));
        call_vm_handler(container, lreg_address);
    }

    if(op_mem.scale != 0)
    {
        //mov VTEMP, imm    ;
        //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
        //jmp VM_MUL        ; multiply INDEX * SCALE
        //vmscratch         ; ignore the rflags we just modified
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(op_mem.scale)));
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
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VTEMP, op_mem.disp.value, 8)));
        call_vm_handler(container, push_address);
    }

    // by default, this will be dereferenced and we will get the value at the address,
    reg_size target_size = reg_size(instruction.operands[index].size / 8);
    if(!(first_operand_as_ea == true && index == 0))
    {
        // this means we are working with the second operand
        auto target_temp = zydis_helper::get_bit_version(VTEMP, target_size);

        call_vm_handler(container, pop_address);
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(target_temp), ZMEMBD(target_temp, 0, target_size)));
        call_vm_handler(container, push_address);

        stack_disp += target_size;
    }
    else
    {
        stack_disp += bit64;
    }

    return encode_status::success;
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dptr op_ptr, int& stack_disp)
{
    // not a supported operand
    return encode_status::unsupported;
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dimm op_imm, int& stack_disp)
{
    auto imm = op_imm.value;
    const auto r_size = static_cast<reg_size>(instruction.instruction.operand_width / 8);

    const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];

    const auto desired_temp_reg = zydis_helper::get_bit_version(VTEMP, r_size);
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(desired_temp_reg), ZIMMU(imm.u)));

    call_vm_handler(container, push_handler->get_handler_va(r_size));

    stack_disp += r_size;
    return encode_status::success;
}