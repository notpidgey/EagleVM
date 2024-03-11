#include "virtual_machine/vm_inst_handlers.h"

#include "virtual_machine/handlers/include_ia32.h"
#include "virtual_machine/handlers/include_vm.h"

vm_inst_handlers::vm_inst_handlers()
{
}

vm_inst_handlers::vm_inst_handlers(vm_inst_regs* push_order)
{
    rm_ = push_order;
}

void vm_inst_handlers::setup_vm_mapping()
{
    v_handlers[MNEMONIC_VM_ENTER] = new vm_enter_handler(rm_, this);
    v_handlers[MNEMONIC_VM_EXIT] = new vm_exit_handler(rm_, this);
    v_handlers[MNEMONIC_VM_LOAD_REG] = new vm_load_handler(rm_, this);
    v_handlers[MNEMONIC_VM_STORE_REG] = new vm_store_handler(rm_, this);
    v_handlers[MNEMONIC_VM_POP_RFLAGS] = new vm_pop_rflags_handler(rm_, this);
    v_handlers[MNEMONIC_VM_PUSH_RFLAGS] = new vm_push_rflags_handler(rm_, this);
    v_handlers[MNEMONIC_VM_TRASH_RFLAGS] = new vm_trash_rflags_handler(rm_, this);

    inst_handlers[ZYDIS_MNEMONIC_PUSH] = new ia32_push_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_POP] = new ia32_pop_handler(rm_, this);

    inst_handlers[ZYDIS_MNEMONIC_IMUL] = new ia32_imul_handler(rm_, this);
    // inst_handlers[ZYDIS_MNEMONIC_MUL] = new ia32_mul_handler(rm_, this);
    // inst_handlers[ZYDIS_MNEMONIC_DIV] = new ia32_div_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_INC] = new ia32_inc_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_DEC] = new ia32_dec_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_ADD] = new ia32_add_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_SUB] = new ia32_sub_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_MOV] = new ia32_mov_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_MOVSX] = new ia32_movsx_handler(rm_, this);
    inst_handlers[ZYDIS_MNEMONIC_LEA] = new ia32_lea_handler(rm_, this);
}