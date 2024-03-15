#include "eaglevm-core/virtual_machine/handlers/vm_handlers/vm_enter.h"

void vm_enter_handler::construct_single(function_container& container, reg_size size, uint8_t operands)
{
    // TODO: this is a temporary fix before i add stack overrun checks
    // we allocate the registers for the virtual machine 20 pushes after the current stack
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RSP), ZMEMBD(GR_RSP, -(8 * vm_overhead), 8)));

    // pushfq
    {
        container.add(zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ));
    }

    // push r0-r15 to stack
    rm_->enumerate(
        [&container](short reg)
        {
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_PUSH, ZREG(reg)));
        });

    // reserve VM call stack
    // reserve VM stack

    // pushfq
    // push r0-r15

    // mov VSP, rsp         ; begin virtualization by setting VSP to rsp
    // mov VREGS, VSP       ; set VREGS to currently pushed stack items
    // mov VCS, VSP         ; set VCALLSTACK to current stack top

    // lea rsp, [rsp + stack_regs + 1] ; this allows us to move the stack pointer in such a way that pushfq overwrite rflags on the stack

    // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead))] ; load the address of where return address is located
    // mov VTEMP, [VTEMP]   ; load actual value into VTEMP
    // lea VCS, [VCS - 8]   ; allocate space to place return address
    // mov [VCS], VTEMP     ; put return address onto call stack

    {

        // TODO: for instruction obfuscation, for operands that use MEM, manually expand them out store it in a new register such as VEMEM and then just dereference that register !!!
        container.add({
            zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VSP), ZREG(GR_RSP)),
            zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VREGS), ZREG(VSP)),
            zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCS), ZREG(VSP)),

            zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RSP), ZMEMBD(VREGS, 8 * (vm_stack_regs), 8)),

            zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead), 8)),
            zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZMEMBD(VTEMP, 0, 8)),
            zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, -8, 8)),
            zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VCS, 0, 8), ZREG(VTEMP)),
        });

        // lea VIP, [0x14000000]    ; load base
        code_label* rel_label = code_label::create();
        container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VBASE), ZMEMBD(IP_RIP, -rel_label->get(), 8))));


        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp (+1 because we pushed an rva)
        // mov VSP, VTEMP

        container.add({
            zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead + 1), 8)),
            zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VSP), ZREG(VTEMP)),
        });
    }

    create_vm_return(container);
}