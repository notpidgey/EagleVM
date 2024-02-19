#pragma once
#include <optional>
#include <array>
#include <functional>

#include "util/util.h"
#include "util/zydis_helper.h"
#include "util/section/function_container.h"

#include "virtual_machine/handlers/vm_handler_context.h"
#include "virtual_machine/base_instruction_virtualizer.h"

#include "virtual_machine/handlers/models/handler_info.h"

#define HANDLER_BUILDER(x) \
[this](function_container& y, reg_size z) \
{ \
    return (x)(y, z); \
}

constexpr uint8_t vm_overhead = 50;
constexpr uint8_t vm_stack_regs = 17;
constexpr uint8_t vm_call_stack = 3;

class code_label;
class base_handler_entry : public base_instruction_virtualizer
{
public:
    explicit base_handler_entry(vm_register_manager* manager, vm_handler_generator* handler_generator);

    function_container construct_handler();
    void initialize_labels();

    virtual void construct_single(function_container& container, reg_size size, uint8_t operands) = 0;

protected:
    ~base_handler_entry() = default;

    bool has_builder_hook;
    bool is_vm_handler;
    function_container container;
    std::vector<handler_info> handlers;
};
