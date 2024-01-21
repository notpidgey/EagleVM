#include "virtual_machine/vm_generator.h"
#include "util/random.h"

#define VIP         rm_.reg_map[I_VIP]
#define VSP         rm_.reg_map[I_VSP]
#define VREGS       rm_.reg_map[I_VREGS]
#define VTEMP       rm_.reg_map[I_VTEMP]
#define VADDR       rm_.reg_map[I_VADDR]
#define PUSHORDER   rm_.reg_stack_order_

#define CREATE_FUNC_JMP(x) \
    zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(IP_RIP, 0, 8)), \
    RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(x))),

vm_generator::vm_generator()
{
    zydis_helper::setup_decoder();
    hg_ = vm_handle_generator(&rm_);
}

void vm_generator::init_reg_order()
{
    rm_.init_reg_order();
}

void vm_generator::init_ran_consts()
{
    hg_.setup_enc_constants();
}

std::pair<uint32_t, std::vector<encode_handler_data>> vm_generator::generate_vm_handlers(bool randomize_handler_position)
{
    uint32_t current_offset = 0;
    hg_.setup_vm_mapping();

    std::vector<encode_handler_data> vm_handlers;
    for (auto& [k, v] : hg_.vm_handlers)
    {
        for (int i = 0; i < 4; i++)
        {
            reg_size supported_size = v.supported_handler_va[i];
            if (supported_size == reg_size::unsupported)
                break;

            instructions_vec instructions = v.creation_binder(supported_size);
            zydis_encoded_instructions encoded_bytes = zydis_helper::encode_queue(instructions);

            vm_handlers.push_back({ current_offset, (uint32_t)encoded_bytes.size(), instructions, encoded_bytes });
            v.handler_va[i] = &std::get<0>(vm_handlers.back());

            current_offset += (uint32_t)encoded_bytes.size();
        }
    }

    // must be 16 byte aligned 
    current_offset = 0;
    for (auto& [handler_offset, handler_size, _, encoded_bytes] : vm_handlers)
    {
        uint8_t remaining_pad = 16 - (handler_size % 16);
        std::vector<uint8_t> padding = create_padding(remaining_pad);

        encoded_bytes += padding;
        handler_size += remaining_pad;

        handler_offset = current_offset;
        current_offset += handler_size;
    }

    if (randomize_handler_position)
    {
        std::random_device random_device;
        std::mt19937 g(random_device());

        std::shuffle(vm_handlers.begin(), vm_handlers.end(), g);

        current_offset = 0;
        for (auto& [handler_offset, handler_size, _1, _2] : vm_handlers)
        {
            handler_offset = current_offset;
            current_offset += handler_size;
        }
    }

    return { current_offset, vm_handlers };
}

#include "util/section/section_manager.h"

void vm_generator::generate_vm_section(bool randomize_handler_position)
{
    section_manager vm_section;
    hg_.setup_vm_mapping();

    for (auto& [k, v] : hg_.vm_handlers)
    {
        function_container func_container;
        for (int i = 0; i < 4; i++)
        {
            code_label* label = func_container.assign_label("vm_handler." + i);

            reg_size supported_size = v.supported_handler_va[i];
            if (supported_size == reg_size::unsupported)
                break;

            instructions_vec instructions = v.creation_binder(supported_size);
            func_container.add(label, instructions);
        }
    }
}

std::vector<zydis_encoder_request> vm_generator::call_vm_enter()
{
    return {};
}

std::vector<zydis_encoder_request> vm_generator::call_vm_exit()
{
    return {};
}

std::pair<bool, std::vector<dynamic_instruction>> vm_generator::translate_to_virtual(const zydis_decode& decoded)
{
    //virtualizer does not support more than 2 operands OR all mnemonics
    if (decoded.instruction.operand_count > 2 || !hg_.vm_handlers.contains(decoded.instruction.mnemonic))
    {
        INSTRUCTION_NOT_SUPPORTED:
        return { false, { zydis_helper::decode_to_encode(decoded) } };
    }

    std::vector<dynamic_instruction> virtualized_instruction;
    for (int i = 0; i < decoded.instruction.operand_count; i++)
    {
        encode_data encode;
        switch (const zydis_decoded_operand operand = decoded.operands[i]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                encode = encode_operand(decoded, operand.reg);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                encode = encode_operand(decoded, operand.mem);
                break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                encode = encode_operand(decoded, operand.ptr);
                break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                encode = encode_operand(decoded, operand.imm);
                break;
        }

        if (encode.second != encode_status::success)
            goto INSTRUCTION_NOT_SUPPORTED;

        virtualized_instruction += encode.first;
    }

    return { true, virtualized_instruction };
}

std::vector<uint8_t> vm_generator::create_padding(const size_t bytes)
{
    std::vector<uint8_t> padding;
    padding.reserve(bytes);

    for (int i = 0; i < bytes; i++)
        padding.push_back(ran_device::get().gen_8());

    return padding;
}

instructions_vec vm_generator::create_func_jump(uint32_t address)
{
    return {
        zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(IP_RIP, 0, 8)),
        zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZIMMU(address)),
    };
}

encode_data vm_generator::encode_operand(const zydis_decode& instruction, zydis_dreg op_reg)
{
    const auto[displacement, size] = rm_.get_stack_displacement(op_reg.value);

    const vm_handler_entry & va_of_push_func = hg_.vm_handlers[MNEMONIC_VM_LOAD_REG];
    const auto func_address = va_of_push_func.get_handler_va(zydis_helper::get_reg_size(op_reg.value));

    //this routine will load the register value to the top of the VSTACK
    const std::vector<dynamic_instruction> load_register
        {
            //mov VTEMP, -8
            //call VM_LOAD_REG
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMS(displacement)),
            RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(func_address))),
        };

    return { load_register, encode_status::success };
}

encode_data vm_generator::encode_operand(const zydis_decode& instruction, zydis_dmem op_mem)
{
    //most definitely riddled with bugs
    if (op_mem.type != ZYDIS_MEMOP_TYPE_MEM)
        return {{}, encode_status::unsupported};

    std::vector<dynamic_instruction> translated_mem;

    const auto[base_displacement, base_size] = rm_.get_stack_displacement(op_mem.base);
    const auto desired_temp_reg = zydis_helper::get_bit_version(VTEMP, base_size);

    const vm_handler_entry& lreg_handler = hg_.vm_handlers[MNEMONIC_VM_LOAD_REG];
    const auto lreg_address = lreg_handler.get_handler_va(base_size);

    const vm_handler_entry& push_handler = hg_.vm_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto push_address = push_handler.get_handler_va(base_size);

    const vm_handler_entry& mul_handler = hg_.vm_handlers[ZYDIS_MNEMONIC_MUL];
    const auto mul_address = mul_handler.get_handler_va(base_size);

    const vm_handler_entry& add_handler = hg_.vm_handlers[ZYDIS_MNEMONIC_ADD];
    const auto add_address = add_handler.get_handler_va(base_size);

    const vm_handler_entry& pop_handler = hg_.vm_handlers[ZYDIS_MNEMONIC_POP];
    const auto pop_address = pop_handler.get_handler_va(base_size);

    //[base + index * scale + disp]

    //1. begin with loading the base register
    //mov VTEMP, imm
    //jmp VM_LOAD_REG
    translated_mem =
        {
            RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(desired_temp_reg), ZLABEL(lreg_address))),
            RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(lreg_address)))
        };

    if (op_mem.scale != 0)
    {
        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        //mov VTEMP, imm    ;
        //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
        //jmp VM_MUL        ; multiply INDEX * SCALE
        //jmp VM_ADD
        const auto[index_displacement, index_size] = rm_.get_stack_displacement(op_mem.index);
        translated_mem +=
            {
                zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(desired_temp_reg), ZIMMU(index_displacement)),
                RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(lreg_address))),
                zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(desired_temp_reg), ZIMMU(op_mem.scale)),
                CREATE_FUNC_JMP(push_address)
                CREATE_FUNC_JMP(mul_address)
                CREATE_FUNC_JMP(add_address)
            };
    }

    if (op_mem.disp.has_displacement)
    {
        //3. load the displacement and add
        //mov VTEMP, imm
        //jmp VM_ADD
        translated_mem +=
            {
                zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMS(op_mem.disp.value)),
                CREATE_FUNC_JMP(add_address)
            };
    }

    //at the end of this, the address inside [] is fully evaluated, now it needs to be transferred to register VTEMP
    //to get whatever is inside that address we just dereference it, now its in VTEMP
    translated_mem +=
        {
            CREATE_FUNC_JMP(pop_address)
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(desired_temp_reg), ZMEMBD(desired_temp_reg, 0, base_size)),
            CREATE_FUNC_JMP(push_address)
        };

    return { translated_mem, encode_status::success };
}

encode_data vm_generator::encode_operand(const zydis_decode& instruction, zydis_dptr op_ptr)
{
    // ¯\_(ツ)_/¯
    return {{}, encode_status::unsupported};
}

encode_data vm_generator::encode_operand(const zydis_decode& instruction, zydis_dimm op_imm)
{
    auto imm = op_imm.value;
    const auto r_size = reg_size(instruction.operands[0].size);

    const vm_handler_entry& va_of_push_func = hg_.vm_handlers[ZYDIS_MNEMONIC_PUSH];
    const auto func_address_mem = va_of_push_func.get_handler_va(r_size);

    std::vector<dynamic_instruction> translated_imm;

    const auto desired_temp_reg = zydis_helper::get_bit_version(VTEMP, r_size);
    translated_imm =
        {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(desired_temp_reg), ZIMMU(imm.u)),
            RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(func_address_mem)))
        };

    return { translated_imm, encode_status::success };
}