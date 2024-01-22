#include "virtual_machine/handlers/ia32_handlers/mov.h"

void ia32_mov_handler::construct_single(function_container& container, reg_size size)
{
}

// bool ia32_mov_handler::hook_builder_operand(const zydis_decode& decoded, dynamic_instructions_vec& instructions,
//                                             int index)
// {
//     const zydis_decoded_operand& operand = decoded.operands[index];
//     if(index == 0)
//     {
//         switch(operand.type)
//         {
//             case ZYDIS_OPERAND_TYPE_REGISTER:
//             {
//                 // since the virtualized mov instruction takes in an address for operand 1
//                 // we need to get the memory location of the register which we are trying to read
//
//                 // mov VTEMP, VREGS ; move address of base VREGS into VTEMP
//                 // sub VTEMP, displacement ; subtract address to get to desired register
//                 // ... jump to vmpush handler
//
//                 // const auto [base_displacement, base_size] = ctx->get_stack_displacement(operand.reg.value);
//
//                 // // need a way to somehow access all other vm handlers from child classes of "vm_handler_entry"
//                 // const vm_handler_entry* push_handler = hg_.vm_handlers[ZYDIS_MNEMONIC_PUSH];
//                 // const auto push_address = push_handler->get_handler_va(base_size);
//
//                 // instructions += {
//                 //     zydis_helper::encode<ZYDIS_MNEMONIC_MOV>(ZREG(VTEMP), ZREG(VREGS)),
//                 //     zydis_helper::encode<ZYDIS_MNEMONIC_SUB>(ZREG(VREGS), base_displacement),
//
//                 //     // ... jump ?
//                 //     zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(IP_RIP, 0, 8)),
//                 //     RECOMPILE(zydis_helper::encode<ZYDIS_MNEMONIC_JMP, zydis_eimm>(ZLABEL(push_address)))
//                 // };
//
//                 break;
//             }
//             case ZYDIS_OPERAND_TYPE_MEMORY:
//             {
//                 // step 1: allow virtualizer to handle the memory operand
//                 // step 2: block the dereference once the event occurs
//
//                 break;
//             }
//             default:
//             {
//                 __debugbreak();
//                 break;
//             }
//         }
//
//         return false;
//     }
//
//     return true;
// }