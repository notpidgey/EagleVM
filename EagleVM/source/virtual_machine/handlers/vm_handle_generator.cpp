#include "virtual_machine/handlers/vm_handler_generator.h"

#include "virtual_machine/handlers/include_ia32.h"
#include "virtual_machine/handlers/include_vm.h"

#include "util/section/code_label.h"

#define VIP         rm_->reg_map[I_VIP]
#define VSP         rm_->reg_map[I_VSP]
#define VREGS       rm_->reg_map[I_VREGS]
#define VTEMP       rm_->reg_map[I_VTEMP]
#define PUSHORDER   rm_->reg_stack_order_
#define RETURN_EXECUTION(x) x.push_back(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(VIP)))

vm_handler_generator::vm_handler_generator()
{
}

vm_handler_generator::vm_handler_generator(vm_register_manager* push_order)
{
    rm_ = push_order;
}

void vm_handler_generator::setup_vm_mapping()
{
    vm_handlers[MNEMONIC_VM_ENTER] = new vm_enter_handler(rm_, this);
    vm_handlers[MNEMONIC_VM_EXIT] = new vm_exit_handler(rm_, this);
    vm_handlers[MNEMONIC_VM_LOAD_REG] = new vm_load_handler(rm_, this);
    vm_handlers[MNEMONIC_VM_STORE_REG] = new vm_store_handler(rm_, this);

    vm_handlers[ZYDIS_MNEMONIC_PUSH] = new ia32_push_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_POP] = new ia32_pop_handler(rm_, this);

    vm_handlers[ZYDIS_MNEMONIC_IMUL] = new ia32_imul_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_MUL] = new ia32_mul_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_DIV] = new ia32_div_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_INC] = new ia32_inc_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_DEC] = new ia32_dec_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_ADD] = new ia32_add_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_SUB] = new ia32_sub_handler(rm_, this);
    vm_handlers[ZYDIS_MNEMONIC_MOV] = new ia32_mov_handler(rm_, this);

    for(const auto& handler : vm_handlers | std::views::values)
        handler->setup_labels();
}

void vm_handler_generator::setup_enc_constants()
{

}
