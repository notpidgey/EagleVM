#include "eaglevm-core/virtual_machine/handlers/vm_handler_context.h"

#include "eaglevm-core/util/random.h"

vm_handler_context::vm_handler_context(vm_inst_regs* context)
{
}

std::vector<zydis_encoder_request> vm_handler_context::generate_handler_return()
{
    return {};
}

void vm_handler_context::setup_keys()
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

    for (unsigned char& value : keys.values)
        value = ran_device::get().gen_8();
}
