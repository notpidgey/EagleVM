#include "virtual_machine/handlers/vm_handle_generator.h"

#include "virtual_machine/handlers/include_ia32.h"
#include "virtual_machine/handlers/include_vm.h"

#include "util/section/code_label.h"

#define VIP         rm_->reg_map[I_VIP]
#define VSP         rm_->reg_map[I_VSP]
#define VREGS       rm_->reg_map[I_VREGS]
#define VTEMP       rm_->reg_map[I_VTEMP]
#define PUSHORDER   rm_->reg_stack_order_
#define RETURN_EXECUTION(x) x.push_back(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(VIP)))

vm_handle_generator::vm_handle_generator() = default;

vm_handle_generator::vm_handle_generator(vm_register_manager* push_order)
{
    rm_ = push_order;
}

void vm_handle_generator::setup_vm_mapping()
{
    vm_handlers[MNEMONIC_VM_ENTER] = vm_enter_handler();
    vm_handlers[MNEMONIC_VM_EXIT] = vm_exit_handler();
    vm_handlers[MNEMONIC_VM_LOAD_REG] = vm_load_handler();
    vm_handlers[MNEMONIC_VM_STORE_REG] = vm_store_handler();

    vm_handlers[ZYDIS_MNEMONIC_PUSH] = vm_push_handler();
    vm_handlers[ZYDIS_MNEMONIC_POP] = vm_pop_handler();
    vm_handlers[ZYDIS_MNEMONIC_MUL] = ia32_mul_handler();
    vm_handlers[ZYDIS_MNEMONIC_DIV] = ia32_div_handler();
    vm_handlers[ZYDIS_MNEMONIC_INC] = ia32_inc_handler();
    vm_handlers[ZYDIS_MNEMONIC_DEC] = ia32_dec_handler();
    vm_handlers[ZYDIS_MNEMONIC_ADD] = ia32_add_handler();
    vm_handlers[ZYDIS_MNEMONIC_SUB] = ia32_sub_handler();
}