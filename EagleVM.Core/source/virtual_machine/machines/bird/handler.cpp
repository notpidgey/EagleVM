#include "eaglevm-core/virtual_machine/machines/bird/handler.h"

#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

namespace eagle::virt::eg
{
    std::vector<ir::base_command_ptr> handler_manager::generate_handler(const codec::mnemonic mnemonic, const uint64_t handler_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];
        return target_mnemonic->gen_handler(handler_sig);
    }
}
