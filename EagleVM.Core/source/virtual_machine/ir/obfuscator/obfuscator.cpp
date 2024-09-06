#include "eaglevm-core/virtual_machine/ir/obfuscator/obfuscator.h"
#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

namespace eagle::ir
{
    void obfuscator::run_preopt_pass(const preopt_block_vec& preopt_vec, const std::unique_ptr<dasm::analysis::liveness>& liveness)
    {
        for (const auto& preopt_block : preopt_vec)
        {
            VM_ASSERT(liveness->live.contains(preopt_block->original_block), "liveness data must contain data for existing block");
            const auto [block_in, block_out] = liveness->live[preopt_block->original_block];
            auto& ir_body = preopt_block->body;

            std::shared_ptr<block_ir> false_in = std::make_shared<block_ir>(vm_block);
            std::shared_ptr<block_ir> false_out = std::make_shared<block_ir>(vm_block);

            for (int k = ZYDIS_REGISTER_RAX; k <= ZYDIS_REGISTER_R15; k++)
            {
                if (0 == block_in.get_gpr64(static_cast<codec::reg>(k)))
                {
                    false_in->push_back(std::make_shared<cmd_push>(util::get_ran_device().gen_16()));
                    false_in->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                }

                if (0 == block_out.get_gpr64(static_cast<codec::reg>(k)))
                {
                    false_out->push_back(std::make_shared<cmd_push>(util::get_ran_device().gen_16()));
                    false_out->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                }
            }

            // false in
            const auto next_cmd_type = ir_body.front()->at(0)->get_command_type();
            if (next_cmd_type == command_type::vm_exec_x86 || next_cmd_type == command_type::vm_exec_dynamic_x86)
                false_in->push_back(std::make_shared<cmd_vm_exit>());

            false_in->push_back(std::make_shared<cmd_branch>(ir_body.front(), exit_condition::jmp));
            ir_body.insert(ir_body.begin(), false_in);

            // false out
            // this code is fucking insane
            VM_ASSERT(ir_body.back()->size() >= 2, "back block must  contain greater than 2 elements");

            const auto back_command_type = ir_body.back()->at(ir_body.size() - 2)->get_command_type();
            if (back_command_type != command_type::vm_exec_dynamic_x86 && back_command_type != command_type::vm_exec_x86)
            {

            }
        }
    }
}
