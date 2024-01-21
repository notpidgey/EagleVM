#include "virtual_machine/handlers/ia32_handlers/mov.h"

ia32_mov_handler::ia32_mov_handler()
{
    supported_sizes = {reg_size::bit64, reg_size::bit32, reg_size::bit16, reg_size::bit8};
}

dynamic_instructions_vec ia32_mov_handler::construct_single(reg_size size)
{
    return {};
}

bool ia32_mov_handler::hook_builder_operand(const zydis_decode& decoded, dynamic_instructions_vec& instructions,
                                            int index)
{
    const zydis_decoded_operand& operand = decoded.operands[index];
    if(index == 0)
    {
        switch(operand.type)
        {
            case ZYDIS_OPERAND_TYPE_REGISTER:
            {
                // since the virtualized mov instruction takes in an address for operand 1
                // we need to get the memory location of the register which we are trying to read

                // mov VTEMP, VREGS ; move address of base VREGS into VTEMP
                // sub VTEMP, displacement ; subtract address to get to desired register
                // ... jump to vmpush handler

                const auto [base_displacement, base_size] = ctx->get_stack_displacement(operand.reg.value);

                const vm_handler_entry* push_handler = hg_.vm_handlers[ZYDIS_MNEMONIC_PUSH];
                const auto push_address = push_handler->get_handler_va(base_size);

                instructions += {
                    zydis_helper::encode<ZYDIS_MNEMONIC_MOV>(ZREG(VTEMP), ZREG(VREGS)),
                    zydis_helper::encode<ZYDIS_MNEMONIC_SUB>(ZREG(VREGS), base_displacement),

                    zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(IP_RIP, 0, 8)),
                    RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(push_address)))
                };

                // i finally thought it through
                // for me to be able to create a jump back to the virtualized code execution after the vm handler runs
                // im going to need a way to know what the code label of the next instruction is going to be
                // but for me to know this i would need to be able to create code labels instead of just pushing instructions
                // this means the vm generate needs to implement a function_container !! 
                std::function<dynamic_instructions_vec(code_label*)> create_jump =
                        [](code_label* label)
                        {
                            code_label* return_label = code_label::create("return_label");
                            return dynamic_instructions_vec{
                                RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(label)))
                            };
                        };

                break;
            }
            case ZYDIS_OPERAND_TYPE_MEMORY:
            {
                // step 1: allow virtualizer to handle the memory operand
                // step 2: block the dereference once the event occurs

                break;
            }
            default:
            {
                __debugbreak();
                break;
            }
        }

        return false;
    }

    return true;
}
