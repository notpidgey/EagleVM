#include "virtual_machine/handlers/vm_handlers/vm_exit.h"

void vm_exit_handler::construct_single(function_container& container, reg_size size)
{
    ZydisEncoderRequest req;
    dynamic_instructions_vec vm_exit_operations;

    //popfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_POPFQ);
        vm_exit_operations.push_back(req);
    }

    //pop r0-r15 to stack
    std::for_each(PUSHORDER.rbegin(), PUSHORDER.rend(),
        [&req, &vm_exit_operations](short reg)
        {
            if(reg == ZYDIS_REGISTER_RSP)
            {
                // this is a very shitty solution ...
                vm_exit_operations += {
                    zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
                    zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(reg), ZIMMU(16)),
                    zydis_helper::encode<ZYDIS_MNEMONIC_POPFQ>()
                };
            }

            req = zydis_helper::encode<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(reg));
            vm_exit_operations.push_back(req);
        });

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, vm_exit_operations.size());
}
