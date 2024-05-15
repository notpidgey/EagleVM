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

    struct tagged_vm_handler
    {
        asmb::code_container_ptr code = nullptr;
        asmb::code_label_ptr label = nullptr;
        uint32_t reference_count = 0;
    };

    using vm_inst_handlers_ptr = std::shared_ptr<class inst_handlers>;

    class inst_handlers
    {
    public:
        explicit inst_handlers(const vm_inst_regs_ptr& push_order);

        void randomize_constants();

        asmb::code_label_ptr get_vm_enter(bool reference = true);
        uint32_t get_vm_enter_reference() const;
        asmb::code_container_ptr build_vm_enter();

        asmb::code_label_ptr get_vm_exit(bool reference = true);
        uint32_t get_vm_exit_reference() const;
        asmb::code_container_ptr build_vm_exit();

        asmb::code_label_ptr get_rlfags_load(bool reference = true);
        uint32_t get_rflags_load_reference() const;
        asmb::code_container_ptr build_rflags_load();

        asmb::code_label_ptr get_rflags_store(bool reference = true);
        uint32_t get_rflags_save_reference() const;
        asmb::code_container_ptr build_rflags_save();

        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, );

        std::vector<asmb::code_container_ptr> get_handlers();

    private:
        vm_inst_regs_ptr inst_regs;

        tagged_vm_handler vm_enter;
        tagged_vm_handler vm_exit;
        tagged_vm_handler vm_rflags_load;
        tagged_vm_handler vm_rflags_save;

        std::unordered_map<tagged_vm_handler, tagged_vm_handler> vm_load;
        std::unordered_map<tagged_vm_handler, tagged_vm_handler> vm_store;

        uint16_t vm_overhead = 8 * 2000;
        uint16_t vm_stack_regs = 17;
        uint16_t vm_call_stack = 3;
    };
}
