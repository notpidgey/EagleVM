#include "virtual_machine/handle_generator.h"

handle_info vm_handle_generator::create_vm_enter()
{
    ZydisEncoderRequest req;
    std::vector<std::vector<uint8_t>> vm_enter_operations;

    //push r0-r15 to stack
    for (int i = ZYDIS_REGISTER_RAX; i <= ZYDIS_REGISTER_R15; i++)
    {
        ZydisReg reg = zydis_helper::create_new_operand<ZydisReg>();
        reg.value = static_cast<ZydisRegister>(i);
        
        std::vector<uint8_t> encoded_instruction = zydis_helper::encode_instruction_1(ZYDIS_MNEMONIC_PUSH, reg);
        vm_enter_operations.push_back(encoded_instruction);
    }

    //pushfq
    {
        std::vector<uint8_t> encoded_instruction = zydis_helper::encode_instruction_0(ZYDIS_MNEMONIC_PUSHFQ);
        vm_enter_operations.push_back(encoded_instruction);
    }

    std::cout << "[+] Created vm_enter with " << vm_enter_operations.size() << " instructions" << std::endl;
    return { vm_enter_operations.size(), vm_enter_operations };
}

handle_info vm_handle_generator::create_vm_exit()
{
    ZydisEncoderRequest req;
    std::vector<std::vector<uint8_t>> vm_exit_operations;

    //pushfq
    {
        std::vector<uint8_t> encoded_instruction = zydis_helper::encode_instruction_0(ZYDIS_MNEMONIC_PUSHFQ);
        vm_exit_operations.push_back(encoded_instruction);
    }

    //push r0-r15 to stack
    for (int i = ZYDIS_REGISTER_R15; i >= ZYDIS_REGISTER_RAX; i--)
    {
        ZydisReg reg = zydis_helper::create_new_operand<ZydisReg>();
        reg.value = static_cast<ZydisRegister>(i);

        std::vector<uint8_t> encoded_instruction = zydis_helper::encode_instruction_1(ZYDIS_MNEMONIC_PUSH, reg);
        vm_exit_operations.push_back(encoded_instruction);
    }

    std::cout << "[+] Created vm_exit with " << vm_exit_operations.size() << " instructions" << std::endl;
    return { vm_exit_operations.size(), vm_exit_operations };
}
