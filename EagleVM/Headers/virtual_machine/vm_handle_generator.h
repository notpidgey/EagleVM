#pragma once

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <functional>
#include <iostream>
#include <cstddef>
#include <array>
#include <map>
#include <ranges>

#include "virtual_machine/models/vm_handler_entry.h"
#include "virtual_machine/vm_register_manager.h"
#include "util/zydis_helper.h"
#include "util/util.h"
#include "util/math.h"

#define HNDL_BIND(x) [&](reg_size s){ return x(s); }

class vm_handle_generator
{
public:
    std::map<int, vm_handler_entry> vm_handlers;

    vm_handle_generator();
    explicit vm_handle_generator(vm_register_manager* push_order);

    void setup_vm_mapping();
    void setup_enc_constants();

    uint32_t get_va_index(const vm_handler_entry& handler, reg_size size);

    ///////////////////
    /// VM SPECIFIC ///
    ///////////////////

    //pushes all registers and RFLAGS to VSTACK
    handle_instructions create_vm_enter(reg_size reg_size = reg_size::bit64);

    //pops all registers from VSTACK to their registers
    handle_instructions create_vm_exit(reg_size reg_size = reg_size::bit64);

    //pushes current value stored in VTMP to the VSTACK
    handle_instructions create_vm_push(reg_size reg_size = reg_size::bit64);

    //pops current value at the top of the stack into VTMP
    handle_instructions create_vm_pop(reg_size reg_size = reg_size::bit64);

    //loads a register value located in VREGS onto the stack
    handle_instructions create_vm_load(reg_size reg_size = reg_size::bit64);

    //jumps from vm_enter into vm function context
    handle_instructions create_vm_enter_jump(uint32_t va_vm_enter, uint32_t va_protected);

    ///////////////////////
    /// GENERAL PURPOSE ///
    ///////////////////////

    //increments value stored at the top of the stack by 1
    handle_instructions create_vm_inc(reg_size reg_size = reg_size::bit64);

    //decrements value stored at the top of the stack by 1
    handle_instructions create_vm_dec(reg_size reg_size = reg_size::bit64);

    //add value stored at the top of the VSTACK to value stored right above it
    handle_instructions create_vm_add(reg_size reg_size = reg_size::bit64);

    //subtract value stored at the top of VSTACK from value stored right above it
    handle_instructions create_vm_sub(reg_size reg_size = reg_size::bit64);

private:
    struct jmp_enc_constants
    {
        uint8_t values[3] = {};
        uint8_t imms[3] = {};
    } keys_;
    vm_register_manager* rm_ = nullptr;
};