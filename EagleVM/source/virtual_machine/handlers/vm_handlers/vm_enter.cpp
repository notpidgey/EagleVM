#include "virtual_machine/handlers/vm_handlers/vm_enter.h"

void vm_enter_handler::construct_single(function_container& container, reg_size size)
{
    // TODO: this is a temporary fix before i add stack overrun checks
    // we allocate the registers for the virtual machine 20 pushes after the current stack
    constexpr auto vm_overhead = 20;
    constexpr auto stack_regs = 17;
    container.add(zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, -(8 * vm_overhead), 8)));

    //pushfq
    {
        container.add( zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ));
    }

    //push r0-r15 to stack
    std::ranges::for_each(PUSHORDER,
        [&container](short reg)
        {
            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_PUSH, zydis_ereg>(ZREG(reg)));
        });

    // reserve VM STACK

    // push r0-r15
    // pushfq

    // mov VSP, rsp         ; begin virtualization by setting VSP to rsp
    // mov VREGS, VSP       ; set VREGS to currently pushed stack items

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

            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 8 * (stack_regs + vm_overhead), 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VRET), ZREG(VTEMP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 8 * (stack_regs + vm_overhead) + 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP), ZREG(VTEMP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_emem>(ZMEMBD(VRET, 0, 8)),
        });
    }

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, container.size());
}