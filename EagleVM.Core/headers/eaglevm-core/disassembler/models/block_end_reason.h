#pragma once

namespace eagle::dasm
{
    enum block_end_reason
    {
        block_end,
        block_jump,
        block_conditional_jump,
        block_ret,
    };
}
