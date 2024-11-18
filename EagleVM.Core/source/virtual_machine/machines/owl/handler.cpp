#include "eaglevm-core/virtual_machine/machines/owl/handler.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

namespace eagle::virt::owl
{
    std::vector<ir::base_command_ptr> handler_manager::generate_handler(const codec::mnemonic mnemonic, const uint64_t handler_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];
        return target_mnemonic->gen_handler(handler_sig);
    }

    std::vector<ir::base_command_ptr> handler_manager::generate_handler(const codec::mnemonic mnemonic, const ir::x86_operand_sig& operand_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];

        ir::op_params sig = { };
        for (const ir::x86_operand& entry : operand_sig)
            sig.emplace_back(entry.operand_type, entry.operand_size);

        const std::optional<uint64_t> handler_id = target_mnemonic->get_handler_id(sig);
        VM_ASSERT(handler_id, "invalid handler, could not be found");

        return target_mnemonic->gen_handler(*handler_id);
    }

    std::vector<ir::base_command_ptr> handler_manager::generate_handler(const codec::mnemonic mnemonic, const ir::handler_sig& handler_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];

        const std::optional<uint64_t> handler_id = target_mnemonic->get_handler_id(handler_sig);
        VM_ASSERT(handler_id, "invalid handler, could not be found");

        return target_mnemonic->gen_handler(*handler_id);
    }
}
