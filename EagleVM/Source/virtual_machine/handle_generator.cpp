#include "virtual_machine/handle_generator.h"

#define VIP         rm_->reg_map[I_VIP]
#define VSP         rm_->reg_map[I_VSP]
#define VREGS       rm_->reg_map[I_VREGS]
#define VTEMP       rm_->reg_map[I_VTEMP]
#define PUSHORDER   rm_->reg_stack_order_

#define TO8(x) zydis_helper::get_bit_version(x, register_size::bit8)
#define TO16(x) zydis_helper::get_bit_version(x, register_size::bit16)
#define TO32(x) zydis_helper::get_bit_version(x, register_size::bit32)

vm_handle_generator::vm_handle_generator() = default;

vm_handle_generator::vm_handle_generator(vm_register_manager* push_order)
{
    rm_ = push_order;
}

handle_instructions vm_handle_generator::create_vm_enter()
{
    ZydisEncoderRequest req;
    std::vector<ZydisEncoderRequest> vm_enter_operations;

    //push r0-r15 to stack
    std::ranges::for_each(PUSHORDER,
          [&req, &vm_enter_operations](short reg)
          {
              req = zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, zydis_ereg>(ZREG(reg));
              vm_enter_operations.push_back(req);
          });

    //pushfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ);

        vm_enter_operations.push_back(req);
    }

    std::cout << "[+] Created vm_enter with " << vm_enter_operations.size() << " instructions" << std::endl;
    return vm_enter_operations;
}

handle_instructions vm_handle_generator::create_vm_exit()
{
    ZydisEncoderRequest req;
    std::vector<ZydisEncoderRequest> vm_exit_operations;

    //popfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_POPFQ);
        vm_exit_operations.push_back(req);
    }

    //pop r0-r15 to stack
    std::for_each(PUSHORDER.rbegin(), PUSHORDER.rend(),
          [&req, &vm_exit_operations](short reg)
          {
              req = zydis_helper::create_encode_request<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(reg));
              vm_exit_operations.push_back(req);
          });

    std::cout << "[+] Created vm_exit with " << vm_exit_operations.size() << " instructions" << std::endl;
    return vm_exit_operations;
}

handle_instructions vm_handle_generator::create_vm_push(register_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions vm_push;

    //sub VSP, reg_size
    //mov [vsp], VTEMP
    if(reg_size == register_size::bit64)
    {
        vm_push = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP))
        };
    }
    else if (reg_size == register_size::bit32)
    {
        vm_push = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP)))
        };
    }
    else if (reg_size == register_size::bit16)
    {
        vm_push = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP)))
        };
    }

    return vm_push;
}

handle_instructions vm_handle_generator::create_vm_pop(register_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions vm_pop;

    //mov VTEMP, [vsp]
    //add VSP, reg_size
    if(reg_size == register_size::bit64)
    {
        vm_pop = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == register_size::bit32)
    {
        vm_pop = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == register_size::bit16)
    {
        vm_pop = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    return vm_pop;
}

handle_instructions vm_handle_generator::create_vm_inc()
{
    handle_instructions vm_inc
    {

    };

    return handle_instructions();
}

handle_instructions vm_handle_generator::create_vm_dec()
{
    handle_instructions vm_dec
    {

    };

    return handle_instructions();
}

handle_instructions vm_handle_generator::create_vm_add()
{
    handle_instructions vm_add
    {

    };

    return handle_instructions();
}

handle_instructions vm_handle_generator::create_vm_mov()
{
    handle_instructions vm_mov
    {

    };

    return handle_instructions();
}