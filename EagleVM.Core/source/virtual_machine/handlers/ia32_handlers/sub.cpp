#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/sub.h"

namespace eagle::virt::handle
{
    void ia32_sub_handler::construct_single(asmbl::function_container& container, reg_size reg_size, uint8_t operands, handler_override override,
        bool inlined)
    {
        uint64_t size = reg_size;

        const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
        const zydis_register target_temp = zydis_helper::get_bit_version(VTEMP, reg_size);

        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));

        // sub qword ptr [VSP], VTEMP       ; subtracts topmost value from 2nd top most value
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_SUB, ZMEMBD(VSP, 0, size), ZREG(target_temp)));

        if (!inlined)
            create_vm_return(container);
    }

    void ia32_sub_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, asmbl::function_container& container)
    {
        {
            const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_LOAD];
            call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));

            inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

            // accept changes to rflags
            const vm_handler_entry* rflag_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_ACCEPT];
            call_vm_handler(container, rflag_handler->get_vm_handler_va(bit64));
        }

        const reg_size target_size = static_cast<reg_size>(decoded_instruction.instruction.operand_width / 8);

        // pop VTEMP2, this will contain the value which we will write
        const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
        call_vm_handler(container, pop_handler->get_handler_va(target_size, 1));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZREG(VTEMP)));

        const zyids_operand_t op_type = decoded_instruction.operands[0].type;
        if (op_type == ZYDIS_OPERAND_TYPE_REGISTER)
        {
            const vm_handler_entry* store_handler = hg_->v_handlers[MNEMONIC_VM_STORE_REG];
            const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];

            // pop VTEMP, this will contain the VREGS offset we will write to
            call_vm_handler(container, pop_handler->get_handler_va(bit32, 1));

            // at this current state:
            // VTEMP: VREGS offset
            // VTEMP2: actual value
            // but we need to get VTEMP2 onto the stack so we can call VM_STORE because
            // this is extremely inefficient and could potentially help with identifying the routine

            // we need to swap VTEMP <-> VTEMP2
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_XCHG, ZREG(VTEMP), ZREG(VTEMP2)));

            // at this current state:
            // VTEMP2: VREGS offset
            // VTEMP: actual value
            call_vm_handler(container, push_handler->get_handler_va(target_size, 1));

            // store handler parameter is an offset of VREGS stored in VTEMP
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZREG(VTEMP2)));

            // call store
            // pops from top of stack
            call_vm_handler(container, store_handler->get_vm_handler_va(target_size));

            // TODO: all of this can be fixed by having a variant handler for STORE/POP which store in different VTEMP values
            // get_vm_handler_va needs to have an override to get a different kind of variant handler for optimization
        }
        else if (op_type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            // pop VTEMP, this will contain the address we will write to
            call_vm_handler(container, pop_handler->get_handler_va(bit64, 1));

            // mov [address], value
            const zydis_register target_temp = zydis_helper::get_bit_version(VTEMP, target_size);
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VTEMP, 0, target_size), ZREG(target_temp)));
        }
        else
        {
            // this should not happen
            __debugbreak();
        }
    }

    int ia32_sub_handler::get_op_action(const zydis_decode& inst, const zyids_operand_t op_type, const int index)
    {
        // we do this because add handler only takes values, but we want to write it
        // if its a memory operand, the action should load value and address
        // if its a register operand, the action should load value and offset
        if (index == 0)
        {
            switch (op_type)
            {
                case ZYDIS_OPERAND_TYPE_REGISTER:
                    return action_value | action_reg_offset;
                case ZYDIS_OPERAND_TYPE_MEMORY:
                    return action_value | action_address;
                default:
                    __debugbreak();
                    break;
            }
        }

        return inst_handler_entry::get_op_action(inst, op_type, index);
    }
}
