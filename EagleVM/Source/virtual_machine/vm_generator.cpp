#include "virtual_machine/vm_generator.h"

#define VIP         rm_.reg_map[I_VIP]
#define VSP         rm_.reg_map[I_VSP]
#define VREGS       rm_.reg_map[I_VREGS]
#define VTEMP       rm_.reg_map[I_VTEMP]
#define PUSHORDER   rm_.reg_stack_order_

#define SR_RAX ZYDIS_REGISTER_RAX
#define SR_RCX ZYDIS_REGISTER_RCX
#define SR_RDX ZYDIS_REGISTER_RDX
#define SR_RBX ZYDIS_REGISTER_RBX
#define SR_RSP ZYDIS_REGISTER_RSP
#define SR_RBP ZYDIS_REGISTER_RBP
#define SR_RSI ZYDIS_REGISTER_RSI
#define SR_RDI ZYDIS_REGISTER_RDI
#define SR_R8  ZYDIS_REGISTER_R8
#define SR_R9  ZYDIS_REGISTER_R9
#define SR_R10 ZYDIS_REGISTER_R10
#define SR_R11 ZYDIS_REGISTER_R11
#define SR_R12 ZYDIS_REGISTER_R12
#define SR_R13 ZYDIS_REGISTER_R13
#define SR_R14 ZYDIS_REGISTER_R14
#define SR_R15 ZYDIS_REGISTER_R15

vm_generator::vm_generator()
{
    zydis_helper::setup_decoder();
    hg_ = vm_handle_generator(&rm_);
}

void vm_generator::init_reg_order()
{
    rm_.init_reg_order();
}

void vm_generator::init_vreg_map()
{
}

void vm_generator::init_ran_consts()
{
    //encryption
    //rol r15, key1
    //sub r15, key2
    //ror r15, key3

    //decryption
    //rol r15, key3
    //add r15, key2
    //ror r15, key1

    //this should be dynamic and more random.
    //in the future, each mnemonic should have a std::function definition and an opposite
    //this will allow for larger and more complex jmp dec/enc routines

    for (unsigned char & value : func_address_keys_.values)
        value = math_util::create_pran<uint8_t>(1, UINT8_MAX);
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
    for (int i = 0; i < decoded_instruction.operands->element_count; i++)
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
        if(enc_status != encode_status::success)
            break;

        zydis_helper::combine_vec(virtualized_instruction, encode.first);
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

std::vector<uint8_t> vm_generator::create_vm_enter_jump(uint32_t va_protected)
{
    //Encryption
    va_protected = _rotl(va_protected, func_address_keys_.values[0]);
    va_protected -= func_address_keys_.values[1];
    va_protected = _rotr(va_protected, func_address_keys_.values[2]);

    //Decryption
    //va_unprotect = _rotl(va_protected, func_address_keys.values[2]);
    //va_unprotect += func_address_keys.values[1];
    //va_unprotect = _rotr(va_unprotect, func_address_keys.values[0]);

    std::vector encode_requests
    {
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, zydis_eimm>(ZIMMU(va_protected)), //push x
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZIMMU(vm_enter_va_)), //jmp vm_enter
    };
    std::vector<uint8_t> data = zydis_helper::encode_queue(encode_requests);

    for (uint8_t i : data)
        std::cout << std::hex << static_cast<int>(i) << " ";

    return data;
}

std::vector<uint8_t> vm_generator::create_vm_enter()
{
    //vm enter routine:
    // 1. push everything to stack
    const std::vector vm_enter = hg_.create_vm_enter();

    // 2. initialize virtual registers
    const std::vector virtual_registers
    {
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(SR_R15), ZREG(SR_RSP))
    };

    // 3. decrypt function jump address & jump
    const std::vector decrypt_routine
    {
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(SR_R15), ZMEMBD(SR_RSP, 144, 8)), //mov r15, [rsp+144]
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ROL, zydis_ereg, zydis_eimm>(ZREG(SR_R15), ZIMMU(func_address_keys_.values[2])), //rol r15, key3
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(SR_R15), ZIMMU(func_address_keys_.values[1])), //add r15, key2
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ROR, zydis_ereg, zydis_eimm>(ZREG(SR_R15), ZIMMU(func_address_keys_.values[0])), //ror r15, key1
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(SR_R15)) //jmp r15
    };

    auto vm_enter_queue = zydis_helper::combine_vec(vm_enter, decrypt_routine);
    return zydis_helper::encode_queue(vm_enter_queue);
}

encode_data vm_generator::encode_register_operand(const zydis_dreg op_reg)
{
    const auto [displacement, size] = rm_.get_stack_displacement(op_reg.value);
    const std::vector decrypt_routine
    {
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, zydis_emem>(ZMEMBD(VREGS, displacement, size)), //push [REG]
    };

    return {};
}

encode_data vm_generator::encode_memory_operand(zydis_dmem op_mem)
{
    std::vector translated_mem
    {
        zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(8))
    };

    return { translated_mem, encode_status::success };
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

    return { translated_imm, encode_status::success };
}
