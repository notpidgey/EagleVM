#include "virtual_machine/vm_generator.h"

#define VIP         rm_.reg_map[I_VIP]
#define VSP         rm_.reg_map[I_VSP]
#define VREGS       rm_.reg_map[I_VREGS]
#define VTEMP       rm_.reg_map[I_VTEMP]
#define PUSHORDER   rm_.reg_stack_order_

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

void vm_generator::generate_vm_handlers(uint32_t va_of_section)
{
    hg_.setup_vm_mapping();

    uint32_t current_section_offset = 0;
    std::printf("%3s %-17s %-10s\n", "", "handler", "instructions");
    for (auto&[k, v] : hg_.vm_handlers)
    {
        for (int i = 0; i < 4; i++)
        {
            reg_size supported_size = v.supported_handler_va[i];
            if (supported_size == reg_size::unsupported)
                break;

            auto instructions = v.creation_binder(supported_size);
            auto encoded = zydis_helper::encode_queue(instructions);

            section_data_ += encoded;
            v.handler_va[i] = current_section_offset;
            current_section_offset += encoded.size();
        }
    }

    for (int y : section_data_)
    {
        if (!y)
        {
            std::cout << "00 ";
            continue;
        }

        if (y <= 0xe)
            std::cout << "0";

        std::cout << std::hex << y << " ";
    }
}

std::vector<zydis_encoder_request> vm_generator::translate_to_virtual(const zydis_decode& decoded_instruction)
{
    if (decoded_instruction.operands->element_count > 2)
    {
        INSTRUCTION_NOT_SUPPORTED:
        return {};
    }

    std::vector<zydis_encoder_request> virtualized_instruction;
    encode_status enc_status = encode_status::success;
    for (int i = decoded_instruction.operands->element_count - 1; i >= 0; i--)
    {
        encode_data encode;
        switch (const zydis_decoded_operand operand = decoded_instruction.operands[i]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                encode = encode_register_operand(operand.reg);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                encode = encode_memory_operand(operand.mem);
                break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                encode = encode_pointer_operand(operand.ptr);
                break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                encode = encode_immediate_operand(operand.imm);
                break;
        }

        enc_status = encode.second;
        if (enc_status != encode_status::success)
            break;

        virtualized_instruction += encode.first;
    }

    return virtualized_instruction;
}

std::vector<uint8_t> vm_generator::create_padding(const size_t bytes)
{
    std::vector<uint8_t> padding(bytes);

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT8_MAX);

    for (int i = 0; i < bytes; i++)
        padding.push_back(dist(rng));

    return padding;
}

encode_data vm_generator::encode_register_operand(const zydis_dreg op_reg)
{
    const auto[displacement, size] = rm_.get_stack_displacement(op_reg.value);
    const vm_handler_entry& va_of_push_func = hg_.vm_handlers[2];
    const auto func_address = hg_.get_va_index(va_of_push_func, zydis_helper::get_reg_size(op_reg.value));

    //this routine will load the register value on top of the stack
    const std::vector load_register
        {
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(displacement)),
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_CALL, zydis_eimm>(ZIMMU(func_address)),
        };

    return {load_register, encode_status::success};
}

encode_data vm_generator::encode_memory_operand(zydis_dmem op_mem)
{
    std::vector translated_mem
        {
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(8))
        };

    return {translated_mem, encode_status::success};
}

encode_data vm_generator::encode_pointer_operand(zydis_dptr op_ptr)
{
    return {};
}

encode_data vm_generator::encode_immediate_operand(zydis_dimm op_imm)
{
    auto imm = op_imm.value;
    std::vector translated_imm //this is a virtual stack push
        {
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, zydis_emem, zydis_eimm>(ZMEMBD(VSP, 0, 8), ZIMMU(imm.u)),   //mov [vsp], IMM
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(8))                 //sub VSP, 8
        };

    return {translated_imm, encode_status::success};
}