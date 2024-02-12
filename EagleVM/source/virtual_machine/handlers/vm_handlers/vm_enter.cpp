#include "virtual_machine/handlers/vm_handlers/vm_enter.h"

void vm_enter_handler::construct_single(function_container& container, reg_size size)
{
    // TODO: this is a temporary fix before i add stack overrun checks
    // we allocate the registers for the virtual machine 20 pushes after the current stack
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, -(8 * vm_overhead), 8)));

    //pushfq
    {
        container.add(zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ));
    }

    //push r0-r15 to stack
    std::ranges::for_each(PUSHORDER,
        [&container](short reg)
        {
            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_PUSH, zydis_ereg>(ZREG(reg)));
        });

    // reserve VM STACK

    // pushfq
    // push r0-r15

    // mov VSP, rsp         ; begin virtualization by setting VSP to rsp
    // mov VREGS, VSP       ; set VREGS to currently pushed stack items

    // lea rsp, [rsp + stack_regs] ; this allows us to move the stack pointer in such a way that pushfq overwrite rflags on the stack

    // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead))] ; load the address of where return address is located
    // mov VRET, VTEMP      ; get address of return address

    // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp
    // mov VSP, VTEMP

    // jmp [VRET]

    {

        // TODO: for instruction obfuscation, for operands that use MEM, manually expand them out store it in a new register such as VEMEM and then just dereference that register !!!
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP), ZREG(GR_RSP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VREGS), ZREG(VSP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8 * (stack_regs), 8)),

            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 8 * (stack_regs + vm_overhead), 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VRET), ZREG(VTEMP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 8 * (stack_regs + vm_overhead) + 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP), ZREG(VTEMP)),

            // from here on, this is all dogshit code so i can do relative jmp
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VRET), ZMEMBD(VRET, 0, 8)),
        });

        // please i beg someone can anyone think of anything better
        code_label* rel_label = code_label::create("vm_enter_rel");
        container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP2), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP2), ZMEMBI(VTEMP2, VRET, 1, 8)));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VTEMP2)));
    }

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, container.size());
}