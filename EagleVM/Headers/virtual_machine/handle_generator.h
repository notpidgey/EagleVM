#pragma once
#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <cstddef>
#include <array>

#include "virtual_machine/vm_register_manager.h"
#include "util/zydis_helper.h"

typedef std::vector<zydis_encoder_request> handle_instructions;
class vm_handle_generator
{
public:
    vm_handle_generator();
    explicit vm_handle_generator(vm_register_manager* push_order);

	handle_instructions create_vm_enter();
	handle_instructions create_vm_exit();

    //pushes current value stored in VTMP to the virtual stack
    //decrements VSP by 8
    handle_instructions create_vm_push(register_size reg_size = register_size::bit64);

    //pops current value at the top of the stack into VTMP
    //increments VSP by 8
    handle_instructions create_vm_pop(register_size reg_size = register_size::bit64);

    handle_instructions create_vm_inc();
    handle_instructions create_vm_dec();
    handle_instructions create_vm_add();
    handle_instructions create_vm_mov();

private:
    vm_register_manager* rm_ = nullptr;
};