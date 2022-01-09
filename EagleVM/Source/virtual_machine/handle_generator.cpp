#include "virtual_machine/handle_generator.h"

handle_instructions vm_handle_generator::create_vm_enter(std::vector<short>& push_order)
{
    ZydisEncoderRequest req;
    std::vector<ZydisEncoderRequest> vm_enter_operations;

    //push r0-r15 to stack
    std::for_each(push_order.begin(), push_order.end(),
        [&req, &vm_enter_operations](short reg)
        {
            req = zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, ZydisReg>(ZREG(reg));

            vm_enter_operations.push_back(req);
        });

    //pushfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ);

        vm_enter_operations.push_back(req);
    }

    std::cout << "[+] Created vm_enter with " << vm_enter_operations.size() << " instructions" << std::endl;
    return vm_enter_operations;
}

handle_instructions vm_handle_generator::create_vm_exit(std::vector<short>& push_order)
{
    ZydisEncoderRequest req;
    std::vector<ZydisEncoderRequest> vm_exit_operations;

    //pushfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_POPFQ);
        
        vm_exit_operations.push_back(req);
    }

    //pop r0-r15 to stack
    std::for_each(push_order.rbegin(), push_order.rend(),
        [&req, &vm_exit_operations](short reg)
        {
            req = zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, ZydisReg>(ZREG(reg));

            vm_exit_operations.push_back(req);
        });

    std::cout << "[+] Created vm_exit with " << vm_exit_operations.size() << " instructions" << std::endl;
    return vm_exit_operations;
}
