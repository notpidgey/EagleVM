#include "virtual_machine/vm_handle_generator.h"

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
    vm_handlers =
        {
            {MNEMONIC_VM_ENTER,     {HNDL_BIND(create_vm_enter), reg_size::bit64}},
            {MNEMONIC_VM_EXIT,      {HNDL_BIND(create_vm_exit),  reg_size::bit64}},
            {MNEMONIC_VM_LOAD_REG,  {HNDL_BIND(create_vm_load),  reg_size::bit64, reg_size::bit32, reg_size::bit16}},
            {MNEMONIC_VM_STORE_REG, {HNDL_BIND(create_vm_store), reg_size::bit64, reg_size::bit32, reg_size::bit16}},
            {ZYDIS_MNEMONIC_PUSH,   {HNDL_BIND(create_vm_push),  reg_size::bit64, reg_size::bit32, reg_size::bit16}},
            {ZYDIS_MNEMONIC_POP,    {HNDL_BIND(create_vm_pop),   reg_size::bit64, reg_size::bit32, reg_size::bit16}},

            {ZYDIS_MNEMONIC_MUL,    {HNDL_BIND(create_vm_mul),   reg_size::bit64, reg_size::bit32, reg_size::bit16}},
            {ZYDIS_MNEMONIC_DIV,    {HNDL_BIND(create_vm_div),   reg_size::bit64, reg_size::bit32, reg_size::bit16}},
            {ZYDIS_MNEMONIC_INC,    {HNDL_BIND(create_vm_inc),   reg_size::bit64, reg_size::bit32, reg_size::bit16, reg_size::bit8}},
            {ZYDIS_MNEMONIC_DEC,    {HNDL_BIND(create_vm_dec),   reg_size::bit64, reg_size::bit32, reg_size::bit16, reg_size::bit8}},
            {ZYDIS_MNEMONIC_ADD,    {HNDL_BIND(create_vm_add),   reg_size::bit64, reg_size::bit32, reg_size::bit16}},
            {ZYDIS_MNEMONIC_SUB,    {HNDL_BIND(create_vm_sub),   reg_size::bit64, reg_size::bit32, reg_size::bit16}},
        };
}

void vm_handle_generator::setup_enc_constants()
{
    //encryption
    //rol r15, key1
    //sub r15, key2
    //ror r15, key3

    //decryption
    //rol r15, key3
    //add r15, key2
    //ror r15, key1

    //this should be dynamic and more random.
    //in the future, each mnemonic should have a std::function definition and an opposite
    //this will allow for larger and more complex jmp dec/enc routines

    for (unsigned char& value : keys_.values)
        value = math_util::create_pran<uint8_t>(1, UINT8_MAX);
}

uint32_t vm_handle_generator::get_va_index(const vm_handler_entry& handler, reg_size size)
{
    if (std::ranges::find(handler.supported_handler_va, size) == std::end(handler.supported_handler_va))
        return -1;

    switch (size)
    {
        case bit64:
            return *handler.handler_va[0];
        case bit32:
            return *handler.handler_va[1];
        case bit16:
            return *handler.handler_va[2];
        case bit8:
            return *handler.handler_va[3];
        default:
            return -1;
    }
}


handle_instructions vm_handle_generator::create_vm_enter(reg_size)
{
    ZydisEncoderRequest req;
    handle_instructions vm_enter_operations;

    //push r0-r15 to stack
    std::ranges::for_each(PUSHORDER,
        [&req, &vm_enter_operations](short reg)
            {
                req = zydis_helper::encode<ZYDIS_MNEMONIC_PUSH, zydis_ereg>(ZREG(reg));
                vm_enter_operations.push_back(req);
            });

    //pushfq
    {
        req = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_PUSHFQ);
        vm_enter_operations.push_back(req);
    }

    //jump decryption routine
    {
        //mov VSP, rsp              ; set VSP to current stack pointer

        //mov VREGS, rsp
        //add VREGS, 136            ; gets VREGS which is 17 pushes behind

        //mov VTEMP, [rsp-144]      ; the first item to be pushed on the stack was a constant value ( 18 pushes behind )
        //rol VTEMP, key3
        //add VTEMP, key2
        //ror VTEMP, key1
        //jmp VTEMP                 ; jump to vm routine for code section

        handle_instructions decryption_routine = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VSP), ZREG(GR_RSP)),

            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VREGS), ZREG(GR_RSP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VREGS), ZIMMU(8 * 17)),

            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(GR_RSP, +(8 * 18), 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ROL, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(keys_.values[2])),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(keys_.values[1])),
            zydis_helper::encode<ZYDIS_MNEMONIC_ROR, zydis_ereg, zydis_eimm>(ZREG(VTEMP), ZIMMU(keys_.values[0])),
            zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_ereg>(ZREG(VTEMP)),
        };

        vm_enter_operations += decryption_routine;
    }

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, vm_enter_operations.size());
    return vm_enter_operations;
}

handle_instructions vm_handle_generator::create_vm_exit(reg_size)
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
                req = zydis_helper::encode<ZYDIS_MNEMONIC_POP, zydis_ereg>(ZREG(reg));
                vm_exit_operations.push_back(req);
            });

    std::printf("%3c %-17s %-10zi\n", 'Q', __func__, vm_exit_operations.size());
    return vm_exit_operations;
}

handle_instructions vm_handle_generator::create_vm_load(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    if (reg_size == reg_size::bit64)
    {
        //mov VTEMP, [VREGS+VTEMP]      ; get address of register
        //mov VTEMP, qword ptr [VTEMP]  ; get register value
        //sub VSP, size                 ; increase VSP
        //mov [VSP], VTEMP              ; push value to stack
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VREGS, 0, reg_size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VREGS, 0, reg_size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP)))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VREGS, 0, reg_size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP)))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}

handle_instructions vm_handle_generator::create_vm_store(reg_size reg_size)
{
    return {};
}

handle_instructions vm_handle_generator::create_vm_enter_jump(uint32_t va_vm_enter, uint32_t va_protected)
{
    //Encryption
    va_protected = _rotl(va_protected, keys_.values[0]);
    va_protected -= keys_.values[1];
    va_protected = _rotr(va_protected, keys_.values[2]);

    //Decryption
    //va_unprotect = _rotl(va_protected, func_address_keys.values[2]);
    //va_unprotect += func_address_keys.values[1];
    //va_unprotect = _rotr(va_unprotect, func_address_keys.values[0]);

    handle_instructions encode_requests
        {
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSH, zydis_eimm>(ZIMMU(va_protected)), //push x
            zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZIMMU(va_vm_enter)), //jmp vm_enter
        };

    return encode_requests;
}

handle_instructions vm_handle_generator::create_vm_push(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    //sub VSP, reg_size
    //mov [vsp], VTEMP
    if (reg_size == reg_size::bit64)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP)))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP)))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}

handle_instructions vm_handle_generator::create_vm_pop(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    //mov VTEMP, [vsp]
    //add VSP, reg_size
    if (reg_size == reg_size::bit64)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}

handle_instructions vm_handle_generator::create_vm_mul(reg_size reg_size)
{
    return handle_instructions();
}

handle_instructions vm_handle_generator::create_vm_div(reg_size reg_size)
{
    return handle_instructions();
}

handle_instructions vm_handle_generator::create_vm_inc(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    //inc [VSP]
    handle_instructions = {
        zydis_helper::encode<ZYDIS_MNEMONIC_INC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}

handle_instructions vm_handle_generator::create_vm_dec(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    //dec [VSP]
    handle_instructions = {
        zydis_helper::encode<ZYDIS_MNEMONIC_DEC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}

handle_instructions vm_handle_generator::create_vm_add(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    if (reg_size == reg_size::bit64)
    {
        //mov VTEMP, [VSP]      ; get top most value on the stack
        //add [VSP+8], VTEMP    ; add topmost value to 2nd top most value
        //pushfq                ; keep track of rflags
        //add VSP, 8            ; pop the top value off of the stack
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(VTEMP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        //mov VTEMP32, [VSP]
        //add [VSP+4], VTEMP32
        //pushfq
        //add VSP, 4
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO32(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        //mov VTEMP16, [VSP]
        //add [VSP+2], VTEMP16
        //pushfq
        //add VSP, 2
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO16(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}

handle_instructions vm_handle_generator::create_vm_sub(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    if (reg_size == reg_size::bit64)
    {
        //mov VTEMP, [VSP]      ; get top most value on the stack
        //sub [VSP+8], VTEMP    ; subtracts topmost value from 2nd top most value
        //pushfq                ; keep track of rflags
        //add VSP, 8            ; pop the top value off of the stack
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(VTEMP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        //mov VTEMP32, [VSP]
        //sub [VSP+4], VTEMP32
        //pushfq
        //add VSP, 4
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO32(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        //mov VTEMP16, [VSP]
        //sub [VSP+2], VTEMP16
        //pushfq
        //add VSP, 2
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO16(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}