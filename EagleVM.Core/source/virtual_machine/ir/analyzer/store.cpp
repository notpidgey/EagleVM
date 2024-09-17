#include "eaglevm-core/virtual_machine/ir/analyzer/store.h"

namespace eagle::ir::analyzer
{
    bool should_release(discrete_store_ptr store, const block_ptr& block, const size_t idx)
    {
        bool found = false;
        for (auto i = idx; i < block->size(); i++)
        {
            auto cmd = block->get_command<base_command>(i);
            switch (cmd->get_command_type())
            {
                // case comma
            }
        }

        return !found;
    }
}
