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
    if (reg_size == register_size::bit64)
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
    if (reg_size == register_size::bit64)
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

handle_instructions vm_handle_generator::create_vm_inc(register_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions vm_inc;

    //inc [VSP]
    vm_inc = {
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_INC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    return vm_inc;
}

handle_instructions vm_handle_generator::create_vm_dec(register_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions vm_dec;

    //dec [VSP]
    vm_dec = {
            zydis_helper::create_encode_request<ZYDIS_MNEMONIC_DEC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    return vm_dec;
}

handle_instructions vm_handle_generator::create_vm_add(register_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions vm_add;

    if (reg_size == register_size::bit64)
    {
        //mov VTEMP, [VSP]      ; get top most value on the stack
        //add [VSP+8], VTEMP    ; add topmost value to 2nd top most value
        //pushfq                ; keep track of rflags
        //add VSP, 8            ; pop the top value off of the stack
        vm_add = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(VTEMP)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSHFQ>(),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == register_size::bit32)
    {
        //mov VTEMP32, [VSP]
        //add [VSP+4], VTEMP32
        //pushfq
        //add VSP, 4
        vm_add = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO32(VTEMP))),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSHFQ>(),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == register_size::bit16)
    {
        //mov VTEMP16, [VSP]
        //add [VSP+2], VTEMP16
        //pushfq
        //add VSP, 2
        vm_add = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO16(VTEMP))),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSHFQ>(),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    return vm_add;
}

handle_instructions vm_handle_generator::create_vm_sub(register_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions vm_sub;

    if (reg_size == register_size::bit64)
    {
        //mov VTEMP, [VSP]      ; get top most value on the stack
        //sub [VSP+8], VTEMP    ; subtracts topmost value from 2nd top most value
        //pushfq                ; keep track of rflags
        //add VSP, 8            ; pop the top value off of the stack
        vm_sub = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(VTEMP)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSHFQ>(),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == register_size::bit32)
    {
        //mov VTEMP32, [VSP]
        //sub [VSP+4], VTEMP32
        //pushfq
        //add VSP, 4
        vm_sub = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO32(VTEMP))),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSHFQ>(),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == register_size::bit16)
    {
        //mov VTEMP16, [VSP]
        //sub [VSP+2], VTEMP16
        //pushfq
        //add VSP, 2
        vm_sub = {
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO16(VTEMP))),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSHFQ>(),
                zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    return vm_sub;
}