#pragma once
#include <optional>
#include <functional>

#include "eaglevm-core/util/util.h"
#include "eaglevm-core/util/zydis_helper.h"

#include "eaglevm-core/compiler/function_container.h"

#include "eaglevm-core/virtual_machine/vm_inst_handlers.h"
#include "eaglevm-core/virtual_machine/vm_inst_regs.h"

#include "eaglevm-core/virtual_machine/vm_inst_regs.h"
#include "eaglevm-core/virtual_machine/handlers/models/handler_info.h"
#include "eaglevm-core/virtual_machine/handlers/models/handler_override.h"

#define VIP         rm_->get_reg(I_VIP)
#define VSP         rm_->get_reg(I_VSP)
#define VREGS       rm_->get_reg(I_VREGS)
#define VTEMP       rm_->get_reg(I_VTEMP)
#define VTEMP2      rm_->get_reg(I_VTEMP2)
#define VCS         rm_->get_reg(I_VCALLSTACK)
#define VCSRET      rm_->get_reg(I_VCSRET)
#define VBASE       rm_->get_reg(I_VBASE)

#define PUSHORDER   rm_->reg_stack_order_

#define HANDLER_BUILDER(x) \
[this](function_container& y, reg_size z) \
{ \
    return (x)(y, z); \
}

namespace eagle::virt::handle
{
    constexpr uint8_t vm_overhead = 100;
    constexpr uint8_t vm_stack_regs = 17;
    constexpr uint8_t vm_call_stack = 3;

    class base_handler_entry
    {
    public:
        explicit base_handler_entry(vm_inst_regs* manager, vm_inst_handlers* handler_generator);

        asmb::function_container construct_handler();
        void initialize_labels();

        void create_vm_return(asmb::function_container& container);
        void call_vm_handler(asmb::function_container& container, asmb::code_label* jump_label);

        virtual void construct_single(asmb::function_container& container, reg_size size, uint8_t operands, handler_override override, bool inlined) = 0;

    protected:
        ~base_handler_entry() = default;

        vm_inst_regs* rm_;
        vm_inst_handlers* hg_;

        bool has_builder_hook;
        bool is_vm_handler;

        asmb::function_container handler_container;
        std::vector<handler_info> handlers;

        void call_virtual_handler(asmb::function_container& container, int handler_id, reg_size size, bool inlined);
        void call_instruction_handler(asmb::function_container& container, zyids_mnemonic handler_id, reg_size size, int operands, bool inlined);

        void push_container(asmb::code_label* label, asmb::function_container& container, zyids_mnemonic mnemonic, auto&&... args)
        {
            container.add(label, zydis_helper::enc(mnemonic, std::forward<decltype(args)>(args)...));
        }

        void push_container(asmb::function_container& container, zyids_mnemonic mnemonic, auto&&... args)
        {
            container.add(zydis_helper::enc(mnemonic, std::forward<decltype(args)>(args)...));
        }
    };
}

