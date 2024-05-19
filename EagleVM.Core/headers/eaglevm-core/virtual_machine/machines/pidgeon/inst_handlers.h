#pragma once
#include <unordered_map>
#include "inst_regs.h"
#include "eaglevm-core/compiler/code_container.h"

namespace eagle::virt::pidg
{
    namespace handle
    {
        using inst_handler_entry_ptr = std::shared_ptr<class inst_handler_entry>;
        using vm_handler_entry_ptr = std::shared_ptr<class vm_handler_entry>;
    }

    using vm_inst_handlers_ptr = std::shared_ptr<class inst_handlers>;
    using machine_ptr = std::shared_ptr<class machine>;

    class inst_handlers
    {
    public:
        explicit inst_handlers(machine_ptr  machine, vm_inst_regs_ptr  push_order);
        void randomize_constants();

        asmb::code_label_ptr get_vm_enter(bool reference = true);
        asmb::code_container_ptr build_vm_enter();

        asmb::code_label_ptr get_vm_exit(bool reference = true);
        asmb::code_container_ptr build_vm_exit();

        asmb::code_label_ptr get_rlfags_load(bool reference = true);
        asmb::code_container_ptr build_rflags_load();

        asmb::code_label_ptr get_rflags_store(bool reference = true);
        asmb::code_container_ptr build_rflags_save();

        asmb::code_label_ptr get_context_load(codec::reg_size size);
        std::vector<asmb::code_container_ptr> build_context_load();

        asmb::code_label_ptr get_context_store(codec::reg_size size);
        std::vector<asmb::code_container_ptr> build_context_store();

        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, uint8_t operand_count, codec::reg_size size);
        std::vector<asmb::code_container_ptr> build_handlers();

    private:
        struct tagged_vm_handler
        {
            asmb::code_container_ptr code;
            asmb::code_label_ptr label;
            bool tagged;

            tagged_vm_handler()
            {
                code = asmb::code_container::create();
                label = asmb::code_label::create();
                tagged = false;
            }
        };

        machine_ptr machine;
        vm_inst_regs_ptr inst_regs;

        tagged_vm_handler vm_enter;
        tagged_vm_handler vm_exit;
        tagged_vm_handler vm_rflags_load;
        tagged_vm_handler vm_rflags_save;

        std::array<tagged_vm_handler, 4> vm_load;
        std::array<tagged_vm_handler, 4> vm_store;

        uint16_t vm_overhead;
        uint16_t vm_stack_regs;
        uint16_t vm_call_stack;
    };
}
