#include "virtual_machine/handlers/vm_handlers/vm_exit.h"

void vm_exit_handler::construct_single(function_container& container, reg_size size)
{
    ZydisEncoderRequest req;

    // we need to place the target RSP after all the pops
    constexpr auto stack_regs = 17;
    container.add({
        zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 8 * (stack_regs + 1), 8)),
        zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VTEMP, 0, 8), ZREG(VSP))
    });

    // mov rsp, VREGS
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(GR_RSP), ZREG(VREGS)));

    //pop r0-r15 to stack
    std::for_each(PUSHORDER.rbegin(), PUSHORDER.rend(),
        [&container](short reg)
        {
            if(reg == ZYDIS_REGISTER_RSP)
            {
                container.add(zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(reg), ZIMMU(8)));
                return;
            }

            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(reg)));
        });

    //popfq
    {
        container.add(zydis_helper::create_encode_request(ZYDIS_MNEMONIC_POPFQ));
    }

    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(GR_RSP)));

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, container.size());
}