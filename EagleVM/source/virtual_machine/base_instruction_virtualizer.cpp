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

    for(int i = 0; i < decoded_instruction.instruction.operand_count; i++)
    {
        encode_status status = encode_status::unsupported;
        switch(const zydis_decoded_operand& operand = decoded_instruction.operands[i]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                status = encode_operand(container, decoded_instruction, operand.reg);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                status = encode_operand(container, decoded_instruction, operand.mem, i);
                break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                status = encode_operand(container, decoded_instruction, operand.ptr);
                break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                status = encode_operand(container, decoded_instruction, operand.imm);
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
    code_label* rel_label = code_label::create("vm_enter_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBI(VIP, VRET, 1, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));
}

void base_instruction_virtualizer::call_vm_handler(function_container& container, code_label* jump_label)
{
    if(!jump_label)
        __debugbreak(); // jump_label should never be null

    // set VRET to return to all the code generated past create_vm_jump
    code_label* retun_label = code_label::create("vm_jump_return_label");
    container.add(RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VRET), ZLABEL(retun_label))));

    // create a jump by jumping BASE + RVA
    code_label* rel_label = code_label::create("vm_jump_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(VIP, jump_label->get(), 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));

    // execution after VM handler should end up here
    container.assign_label(retun_label);
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction,
                                                           zydis_dreg op_reg)
{
    const auto [displacement, size] = rm_->get_stack_displacement(op_reg.value);

    const vm_handler_entry* va_of_push_func = hg_->vm_handlers[MNEMONIC_VM_LOAD_REG];
    const auto func_address = va_of_push_func->get_handler_va(zydis_helper::get_reg_size(op_reg.value));

    //this routine will load the register value to the top of the VSTACK
    //mov VTEMP, -8
    //call VM_LOAD_REG

    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMS(displacement)));
    call_vm_handler(container, func_address);

    return encode_status::success;
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, int index)
{
    //most definitely riddled with bugs
    if(op_mem.type != ZYDIS_MEMOP_TYPE_MEM)
        return encode_status::unsupported;

    const vm_handler_entry* lreg_handler = hg_->vm_handlers[MNEMONIC_VM_LOAD_REG];
    const auto lreg_address = lreg_handler->get_handler_va(bit64);

    const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto push_address = push_handler->get_handler_va(bit64);

    const vm_handler_entry* mul_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_MUL];
    const auto mul_address = mul_handler->get_handler_va(bit64);

    const vm_handler_entry* add_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_ADD];
    const auto add_address = add_handler->get_handler_va(bit64);

    const vm_handler_entry* sub_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_SUB];
    const auto sub_address = sub_handler->get_handler_va(bit64);

    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    const auto pop_address = pop_handler->get_handler_va(bit64);

    //[base + index * scale + disp]

    //1. begin with loading the base register
    //mov VTEMP, imm
    //jmp VM_LOAD_REG
    {
        const auto [base_displacement, base_size] = rm_->get_stack_displacement(op_mem.base);

        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(base_displacement)));
        call_vm_handler(container, lreg_address);
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

    if(op_mem.scale != 1)
    {
        //mov VTEMP, imm    ;
        //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
        //jmp VM_MUL        ; multiply INDEX * SCALE
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(op_mem.scale)));
        call_vm_handler(container, push_address);
        call_vm_handler(container, mul_address);
    }

    if(op_mem.index != ZYDIS_REGISTER_NONE)
    {
        call_vm_handler(container, add_address);
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
    // in the case that we do not, the value at the top of the stack will just be an address
    if(!first_operand_as_ea && index == 0)
    {
        call_vm_handler(container, pop_address);
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VTEMP, 0, 8)));
        call_vm_handler(container, push_address);
    }

    return encode_status::success;
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dptr op_ptr)
{
    // not a supported operand
    return encode_status::unsupported;
}

encode_status base_instruction_virtualizer::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dimm op_imm)
{
    auto imm = op_imm.value;
    const auto r_size = static_cast<reg_size>(instruction.operands[0].size / 8);

    const vm_handler_entry* va_of_push_func = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto func_address_mem = va_of_push_func->get_handler_va(r_size);

    const auto desired_temp_reg = zydis_helper::get_bit_version(VTEMP, r_size);
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(desired_temp_reg), ZIMMU(imm.u)));
    call_vm_handler(container, func_address_mem);

    return encode_status::success;
}