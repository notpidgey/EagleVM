#pragma once

namespace eagle::ir
{
    enum block_action
    {
        none = 0,
        exits_vm = 1,
        enters_vm = 2,
        branches = 4,
    };

    enum block_state
    {
        unassigned,
        vm_block,
        x86_block,
        mixed
    };
}
