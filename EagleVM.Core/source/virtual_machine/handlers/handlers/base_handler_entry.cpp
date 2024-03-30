#include "eaglevm-core/virtual_machine/handlers/handler/base_handler_entry.h"
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"
#include "eaglevm-core/virtual_machine/handlers/handler/vm_handler_entry.h"

#include "eaglevm-core/assembler/code_label.h"

namespace eagle::virt::handle
{
    base_handler_entry::base_handler_entry(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
    {
        hg_ = handler_generator;
        rm_ = manager;

        has_builder_hook = false;
        is_vm_handler = false;

        handler_container = asmbl::function_container();
    }

    asmbl::function_container base_handler_entry::construct_handler()
    {
        std::ranges::for_each(handlers, [this](const handler_info& info)
        {
            const char size = zydis_helper::reg_size_to_string(info.instruction_width);

            handler_container.assign_label(info.target_label);
            info.target_label->set_name(size + std::string(typeid(*this).name()));
            info.target_label->set_comment(true);

            construct_single(handler_container, info.instruction_width, info.operand_count, info.override, false);
        });

        return handler_container;
    }

    void base_handler_entry::initialize_labels()
    {
        for (auto& handler : handlers)
            handler.target_label = asmbl::code_label::create();
    }

    void base_handler_entry::create_vm_return(asmbl::function_container& container)
    {
        // now that we use a virtual call stack we must pop the top address

        // mov VCSRET, [VCS]        ; pop from call stack
        // lea VCS, [VCS + 8]       ; move up the call stack pointer
        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCSRET), ZMEMBD(VCS, 0, 8)));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, 8, 8)));

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBI(VBASE, VCSRET, 1, 8)));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));
    }

    void base_handler_entry::call_vm_handler(asmbl::function_container& container, asmbl::code_label* jump_label)
    {
        if (!jump_label)
            __debugbreak(); // jump_label should never be null

        asmbl::code_label* retun_label = asmbl::code_label::create();

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
        container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VCS, 0, 8), ZLABEL(retun_label))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(VBASE, jump_label->get(), 8))));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));

        // execution after VM handler should end up here
        container.assign_label(retun_label);
    }

    void base_handler_entry::call_virtual_handler(asmbl::function_container& container, int handler_id, reg_size size, bool inlined)
    {
        vm_handler_entry* target_handler = hg_->v_handlers[handler_id];
        if (target_handler == nullptr)
        {
            __debugbreak();
            return;
        }

        if (inlined)
            target_handler->construct_single(container, size, 0, ho_default, true);
        else
            call_vm_handler(container, target_handler->get_vm_handler_va(size));
    }

    void base_handler_entry::call_instruction_handler(asmbl::function_container& container, zyids_mnemonic handler_id, reg_size size, int operands, bool inlined)
    {
        handle::inst_handler_entry* target_handler = hg_->inst_handlers[handler_id];
        if (target_handler == nullptr)
        {
            __debugbreak();
            return;
        }

        if (inlined)
            target_handler->construct_single(container, size, operands, handle::ho_default, true);
        else
            call_vm_handler(container, target_handler->get_handler_va(size, operands));
    }
}