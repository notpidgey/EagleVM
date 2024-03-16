#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"
#include "eaglevm-core/virtual_machine/handlers/handler/vm_handler_entry.h"

std::pair<bool, function_container> inst_handler_entry::translate_to_virtual(const zydis_decode& decoded_instruction,
    const uint64_t original_rva)
{
    function_container container = {};

    const inst_handler_entry* handler = hg_->inst_handlers[decoded_instruction.instruction.mnemonic];
    const code_label* target = handler->get_handler_va(
        static_cast<reg_size>(decoded_instruction.instruction.operand_width / 8),
        decoded_instruction.instruction.operand_count_visible
    );

    if(target == nullptr)
    {
    INSTRUCTION_NOT_SUPPORTED:
        return { false, container };
    }

    container.assign_label(code_label::create(zydis_helper::instruction_to_string(decoded_instruction), true));

    int32_t current_disp = 0;
    for(uint8_t i = 0; i < decoded_instruction.instruction.operand_count_visible; i++)
    {
        encode_status status = encode_status::unsupported;
        container.assign_label(code_label::create(zydis_helper::operand_to_string(decoded_instruction, i), true));

        encode_ctx ctx
        {
            &current_disp,
            original_rva,
            i
        };

        switch(const zydis_decoded_operand& operand = decoded_instruction.operands[i]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                status = encode_operand(container, decoded_instruction, operand.reg, ctx);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                status = encode_operand(container, decoded_instruction, operand.mem, ctx);
                break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                status = encode_operand(container, decoded_instruction, operand.ptr, ctx);
                break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                status = encode_operand(container, decoded_instruction, operand.imm, ctx);
                break;
        }

        if(status != encode_status::success)
            goto INSTRUCTION_NOT_SUPPORTED;
    }

    finalize_translate_to_virtual(decoded_instruction, container);
    return { true, container };
}

code_label* inst_handler_entry::get_handler_va(reg_size width, uint8_t operands) const
{
    auto it = std::ranges::find_if(handlers,
        [width, operands](const handler_info& h)
        {
            if(h.instruction_width == width && h.operand_count == operands)
                return true;

            return false;
        });

    if(it != handlers.end())
        return it->target_label;

    return nullptr;
}

bool inst_handler_entry::virtualize_as_address(const zydis_decode& inst, int index)
{
    return false;
}

void inst_handler_entry::finalize_translate_to_virtual(const zydis_decode& decoded, function_container& container)
{
    code_label* target_handler = get_handler_va(static_cast<reg_size>(decoded.instruction.operand_width / 8), decoded.instruction.operand_count_visible);
    call_vm_handler(container, target_handler);
}

encode_status inst_handler_entry::encode_operand(
    function_container& container, const zydis_decode& instruction,
    zydis_dreg op_reg, encode_ctx& context)
{
    auto [stack_disp, orig_rva, index] = context;

    // what about cases where we have RSP as the register?
    if(virtualize_as_address(instruction, index))
    {
        const auto [displacement, size] = rm_->get_stack_displacement(TO64(op_reg.value));
        const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];

        // this means we want to put the address of of the target register at the top of the stack
        // mov VTEMP, VREGS + DISPLACEMENT
        // push

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VREGS, displacement, 8)));
        call_vm_handler(container, push_handler->get_handler_va(bit64, 1)); // always 64 bit because its an address

        *stack_disp += bit64;
    }
    else
    {
        const auto [displacement, size] = rm_->get_stack_displacement(op_reg.value);
        const vm_handler_entry* load_handler = hg_->v_handlers[MNEMONIC_VM_LOAD_REG];

        // this routine will load the register value to the top of the VSTACK
        // mov VTEMP, -8
        // call VM_LOAD_REG

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMS(displacement)));
        call_vm_handler(container, load_handler->get_vm_handler_va(zydis_helper::get_reg_size(op_reg.value)));

        *stack_disp += zydis_helper::get_reg_size(op_reg.value);
    }

    return encode_status::success;
}

encode_status inst_handler_entry::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, encode_ctx& context)
{
    auto [stack_disp, orig_rva, index] = context;
    if(op_mem.type != ZYDIS_MEMOP_TYPE_MEM && op_mem.type != ZYDIS_MEMOP_TYPE_AGEN)
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
    code_label* rip_label;
    {
        if(op_mem.base == ZYDIS_REGISTER_RSP)
        {
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZREG(VSP)));
            if(stack_disp)
            {
                // this is meant to account for any possible pushes we set up
                // if we now access VSP, its not going to be what it was before we called this function
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VSP, *stack_disp, 8)));
            }

            call_vm_handler(container, push_address);
        }
        else if(op_mem.base == ZYDIS_REGISTER_RIP)
        {
            rip_label = code_label::create("rip: " + std::to_string(orig_rva));

            container.add(rip_label, zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(IP_RIP, 0, 8)));
            call_vm_handler(container, push_address);
        }
        else
        {
            const auto [base_displacement, base_size] = rm_->get_stack_displacement(op_mem.base);

            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, lreg_address);
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

        if(op_mem.base == ZYDIS_REGISTER_RIP)
        {
            // since this is RIP relative we first want to calculate where the original instruction is trying to access
            auto [target, _] = zydis_helper::calc_relative_rva(instruction, orig_rva, index);

            // VTEMP = RIP at first operand instruction
            // target = RIP + constant

            call_vm_handler(container, pop_address);
            container.add([=](uint64_t)
            {
                const uint64_t rip = rip_label->get();
                const uint64_t constant = target - rip;

                return zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VTEMP, constant, 8));
            });

            call_vm_handler(container, push_address);
        }
        else
        {
            // pop current value into VTEMP
            // lea VTEMP, [VTEMP +- imm]
            // push

            call_vm_handler(container, pop_address);
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VTEMP, op_mem.disp.value, 8)));
            call_vm_handler(container, push_address);
        }
    }

    if(op_mem.type != ZYDIS_MEMOP_TYPE_AGEN)
    {
        // by default, this will be dereferenced and we will get the value at the address,
        reg_size target_size = reg_size(instruction.instruction.operand_width / 8);

        // this means we are working with the second operand
        auto target_temp = zydis_helper::get_bit_version(VTEMP, target_size);

        call_vm_handler(container, pop_address);
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(target_temp), ZMEMBD(VTEMP, 0, target_size)));
        call_vm_handler(container, push_handler->get_handler_va(target_size, 1));

        *stack_disp += target_size;
    }
    else
    {
        *stack_disp += bit64;
    }

    return encode_status::success;
}

encode_status inst_handler_entry::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dptr op_ptr, encode_ctx& context)
{
    // not a supported operand
    return encode_status::unsupported;
}

encode_status inst_handler_entry::encode_operand(function_container& container, const zydis_decode& instruction, zydis_dimm op_imm, encode_ctx& context)
{
    auto [stack_disp, orig_rva, index] = context;

    auto imm = op_imm.value;
    const auto r_size = static_cast<reg_size>(instruction.instruction.operand_width / 8);

    const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];

    const auto desired_temp_reg = zydis_helper::get_bit_version(VTEMP, r_size);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(desired_temp_reg), ZIMMU(imm.u)));

    call_vm_handler(container, push_handler->get_handler_va(r_size, 1));

    *stack_disp += r_size;
    return encode_status::success;
}