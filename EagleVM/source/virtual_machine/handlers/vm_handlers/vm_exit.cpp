#include "virtual_machine/handlers/vm_handlers/vm_exit.h"

void vm_exit_handler::construct_single(function_container& container, reg_size size)
{
    // we need to place the target RSP after all the pops
    // lea VTEMP, [VREGS + vm_stack_regs]
    // mov [VTEMP], VSP
    container.add({
        zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VREGS, 8 * vm_stack_regs, 8)),
        zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VTEMP, 0, 8), ZREG(VSP))
    });

    // we also need to setup an RIP to return to main program execution
    // we will place that after the RSP
    code_label* rel_label = code_label::create();
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBI(VIP, VCSRET, 1, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VSP, -8, 8), ZREG(VIP)));

    // mov rsp, VREGS
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(GR_RSP), ZREG(VREGS)));

    //pop r0-r15 to stack
    std::for_each(PUSHORDER.rbegin(), PUSHORDER.rend(),
        [&container](short reg)
        {
            if(reg == ZYDIS_REGISTER_RSP || reg == ZYDIS_REGISTER_RIP)
            {
                container.add(zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8)));
                return;
            }

            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(reg)));
        });

    //popfq
    {
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_POPFQ));
    }

    // the rsp that we setup earlier before popping all the regs
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(GR_RSP)));
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_emem>(ZMEMBD(GR_RSP, -8, 8)));
    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, container.size());
}