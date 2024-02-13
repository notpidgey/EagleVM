#include "virtual_machine/handlers/ia32_handlers/sub.h"

#include "virtual_machine/vm_generator.h"

void ia32_sub_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;

    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    if(reg_size == reg_size::bit64)
    {
        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));

        //sub qword ptr [VSP], VTEMP    ; subtracts topmost value from 2nd top most value
        //pushfq                        ; keep track of rflags
        //lea rsp, [rsp + 8]            ; we overwrote the rflags on the stack by pushing, so we reset
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });

        call_vm_handler(container, pop_handler->get_handler_va(reg_size));
    }
    else if(reg_size == reg_size::bit32)
    {
        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));

        //sub dword ptr [VSP], VTEMP32
        //pushfq
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP))),

            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });

        call_vm_handler(container, pop_handler->get_handler_va(reg_size));
    }
    else if(reg_size == reg_size::bit16)
    {
        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));

        //sub word ptr [VSP], VTEMP16
        //pushfq
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP))),

            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });

        call_vm_handler(container, pop_handler->get_handler_va(reg_size));
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, container.size());
}

void ia32_sub_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    auto operand = decoded_instruction.operands[0];
    switch(operand.type)
    {
        case ZYDIS_OPERAND_TYPE_REGISTER:
        {
            const vm_handler_entry* store_handler = hg_->vm_handlers[MNEMONIC_VM_STORE_REG];
            const auto [base_displacement, reg_size] = rm_->get_stack_displacement(operand.reg.value);

            // the sum is at the top of the stack
            // we can save to the destination register by specifying the displacement
            // and then calling store reg
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, store_handler->get_handler_va(reg_size));
        }
        break;
        case ZYDIS_OPERAND_TYPE_MEMORY:
        {
            const auto reg_size = zydis_helper::get_reg_size(operand.mem.base);
            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(
                ZMEMBD(VTEMP, 0, reg_size),
                ZREG(zydis_helper::get_bit_version(VTEMP, reg_size))
            ));
        }
        break;
        default:
            __debugbreak();
            break;
    }
}