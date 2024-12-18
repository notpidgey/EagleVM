#pragma once

namespace eagle::dasm
{
    enum block_jump_location
    {
        jump_unknown,
        jump_outside_segment,
        jump_inside_segment,
    };
}
