#pragma once
#include <optional>
#include <array>
#include <functional>

#include "util/util.h"
#include "util/zydis_helper.h"
#include "util/section/function_container.h"

#include "virtual_machine/handlers/vm_handler_context.h"

#define VIP         ctx->reg_map[I_VIP]
#define VSP         ctx->reg_map[I_VSP]
#define VREGS       ctx->reg_map[I_VREGS]
#define VTEMP       ctx->reg_map[I_VTEMP]
#define PUSHORDER   ctx->reg_stack_order_
#define RETURN_EXECUTION(x) x.push_back(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(VIP)))

class code_label;
class vm_handler_entry
{
public:
    inline static vm_register_manager* ctx = nullptr;

    vm_handler_entry();

    code_label* get_handler_va(reg_size size) const;
    function_container construct_handler();

    virtual bool hook_builder_init(const zydis_decode& decoded, dynamic_instructions_vec& instruction);
    virtual bool hook_builder_operand(const zydis_decode& decoded, dynamic_instructions_vec& instructions, int index);
    virtual bool hook_builder_finalize(const zydis_decode& decoded, dynamic_instructions_vec& instructions);

protected:
    bool has_builder_hook;
    bool is_vm_handler;
    function_container container;

    std::vector<reg_size> supported_sizes;
    std::unordered_map<reg_size, code_label*> supported_handlers;

    virtual dynamic_instructions_vec construct_single(reg_size size) = 0;
};
