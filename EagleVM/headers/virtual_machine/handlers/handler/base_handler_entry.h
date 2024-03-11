#pragma once
#include <optional>
#include <array>
#include <functional>

#include "util/util.h"
#include "util/zydis_helper.h"
#include "util/section/function_container.h"

#include "virtual_machine/handlers/vm_handler_generator.h"
#include "virtual_machine/handlers/vm_handler_context.h"
#include "virtual_machine/handlers/models/handler_info.h"

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

constexpr uint8_t vm_overhead = 100;
constexpr uint8_t vm_stack_regs = 17;
constexpr uint8_t vm_call_stack = 3;

class code_label;
class base_handler_entry
{
public:
    explicit base_handler_entry(vm_inst_regs* manager, vm_handler_generator* handler_generator);

    function_container construct_handler();
    void initialize_labels();

    void create_vm_return(function_container& container);
    void call_vm_handler(function_container& container, code_label* jump_label);

    virtual void construct_single(function_container& container, reg_size size, uint8_t operands) = 0;

protected:
    ~base_handler_entry() = default;

    bool has_builder_hook;
    bool is_vm_handler;
    function_container container;
    std::vector<handler_info> handlers;
};
