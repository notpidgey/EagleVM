#include "virtual_machine/handlers/vm_handlers/vm_enter.h"

vm_enter_handler::vm_enter_handler()
{
    supported_sizes = { reg_size::bit64 };
}

dynamic_instructions_vec vm_enter_handler::construct_single(reg_size size)
{
    ZydisEncoderRequest req;
    dynamic_instructions_vec vm_enter_operations;

    //push r0-r15 to stack
    std::ranges::for_each(PUSHORDER,
        [&req, &vm_enter_operations](short reg)
        {
            req = zydis_helper::encode<ZYDIS_MNEMONIC_PUSH, zydis_ereg>(ZREG(reg));
            vm_enter_operations.push_back(req);
        });

    //pushfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ);
        vm_enter_operations.push_back(req);
    }

    //jump decryption routine
    {
        //mov VSP, rsp              ; set VSP to current stack pointer

        //mov VREGS, rsp
        //add VREGS, 136            ; gets VREGS which is 17 pushes behind

        //mov VTEMP, [rsp-144]      ; the first item to be pushed on the stack was a constant value ( 18 pushes behind )

        // these will be implemented in the future:
        // rol VTEMP, key3
        // add VTEMP, key2
        // ror VTEMP, key1

        //jmp VTEMP                 ; jump to vm routine for code section

        dynamic_instructions_vec decryption_routine = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP), ZREG(GR_RSP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VREGS), ZREG(GR_RSP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VREGS), ZIMMU(8 * 17)),

            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(GR_RSP, +(8 * 18), 8)),
            // zydis_helper::encode<ZYDIS_MNEMONIC_ROL, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(keys_.values[2])),
            // zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(keys_.values[1])),
            // zydis_helper::encode<ZYDIS_MNEMONIC_ROR, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(keys_.values[0])),
            zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(VTEMP)),
        };

        vm_enter_operations += decryption_routine;
    }

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, vm_enter_operations.size());
    return vm_enter_operations;
}
