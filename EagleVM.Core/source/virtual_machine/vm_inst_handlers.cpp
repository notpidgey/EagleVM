#include "eaglevm-core/virtual_machine/vm_inst_handlers.h"

#include "eaglevm-core/virtual_machine/handlers/include_ia32.h"
#include "eaglevm-core/virtual_machine/handlers/include_vm.h"

#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"
#include "eaglevm-core/virtual_machine/handlers/handler/vm_handler_entry.h"

namespace eagle::virt
{
    vm_inst_handlers::vm_inst_handlers(vm_inst_regs* push_order)
    {
        rm_ = push_order;
    }

    void vm_inst_handlers::setup_vm_mapping()
    {
        v_handlers[MNEMONIC_VM_ENTER] = new handle::vm_enter_handler(rm_, this);
        v_handlers[MNEMONIC_VM_EXIT] = new handle::vm_exit_handler(rm_, this);
        v_handlers[MNEMONIC_VM_LOAD_REG] = new handle::vm_load_handler(rm_, this);
        v_handlers[MNEMONIC_VM_STORE_REG] = new handle::vm_store_handler(rm_, this);
        v_handlers[MNEMONIC_VM_RFLAGS_ACCEPT] = new handle::vm_rflags_save_handler( rm_, this);
        v_handlers[MNEMONIC_VM_RFLAGS_LOAD] = new handle::vm_rflags_load_handler(rm_, this);

        inst_handlers[ZYDIS_MNEMONIC_PUSH] = new handle::ia32_push_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_POP] = new handle::ia32_pop_handler(rm_, this);

        inst_handlers[ZYDIS_MNEMONIC_IMUL] = new handle::ia32_imul_handler(rm_, this);
        // inst_handlers[ZYDIS_MNEMONIC_MUL] = new ia32_mul_handler(rm_, this);
        // inst_handlers[ZYDIS_MNEMONIC_DIV] = new ia32_div_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_INC] = new handle::ia32_inc_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_DEC] = new handle::ia32_dec_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_ADD] = new handle::ia32_add_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_SUB] = new handle::ia32_sub_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_MOV] = new handle::ia32_mov_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_MOVSX] = new handle::ia32_movsx_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_LEA] = new handle::ia32_lea_handler(rm_, this);
        inst_handlers[ZYDIS_MNEMONIC_CMP] = new handle::ia32_cmp_handler(rm_, this);
    }

    handle::inst_handler_entry* vm_inst_handlers::get_inst_handler(const int handler)
    {
        if(inst_handlers.contains(handler))
            return inst_handlers[handler];

        return nullptr;
    }

    handle::vm_handler_entry* vm_inst_handlers::get_vm_handler(const int handler)
    {
        if(v_handlers.contains(handler))
            return v_handlers[handler];

        return nullptr;
    }
}
