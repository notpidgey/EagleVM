#include "virtual_machine/handlers/vm_handlers/vm_enter.h"

void vm_enter_handler::construct_single(function_container& container, reg_size size)
{
    ZydisEncoderRequest req;
    dynamic_instructions_vec vm_enter_operations;

    // TODO: this is a temporary fix before i add stack overrun checks
    // we allocate the registers for the virtual machine 20 pushes after the current stack
    constexpr auto vm_overhead = 20;
    vm_enter_operations.emplace_back(zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, -(8 * vm_overhead), 8)));

    //push r0-r15 to stack
    std::ranges::for_each(PUSHORDER,
        [&req, &vm_enter_operations](short reg)
        {
            req = zydis_helper::encode<ZYDIS_MNEMONIC_PUSH, zydis_ereg>(ZREG(reg));
            vm_enter_operations.emplace_back(req);
        });

    //pushfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ);
        vm_enter_operations.emplace_back(req);
    }

    // reserve VM STACK

    // push r0-r15
    // pushfq

    // mov VSP, rsp         ; begin virtualization by setting VSP to rsp
    // lea VREGS, [VSP + 8] ; get vregs to point to bottom of stack
    // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead))]
    // mov VSP, VTEMP       ; begin using virtual machine VSP
    // 


    {
        //mov VSP, rsp                                          ; set VSP to current stack pointer
        //mov VREGS, VSP                                        ; set VREGSS to bottom of pushed registers
        //lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead))]   ; load address of initial rsp
        //mov VSP, VTEMP                                        ; restore original rsp as VSP
        //sub VSP, 8                                            ; move past return address stored at the top of the stack
        //mov VTEMP, [VTEMP]                                    ; get the return address specified at the top of the stack
        //jmp VTEMP                                             ; jump to vm routine for code section

        // TODO: for instruction obfuscation, for operands that use MEM, manually expand them out store it in a new register such as VEMEM and then just dereference that register !!!
        dynamic_instructions_vec decryption_routine = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP), ZREG(GR_RSP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_ereg>(ZREG(VREGS), ZREG(VSP)),

            // load the address of the stack where the address to where we want to jump to is stored
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 8 * (stack_regs + vm_overhead), 8)),

            // restore the stack pointer before we jump
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP),ZREG(VTEMP)),

            // move past return address
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(8)),

            // dereference the pointer and store it in VTEMP
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP),ZMEMBD(VTEMP, 0, 8)),

            // jump to that address
            zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(VTEMP)),
        };

        vm_enter_operations += decryption_routine;
    }

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, vm_enter_operations.size());
}