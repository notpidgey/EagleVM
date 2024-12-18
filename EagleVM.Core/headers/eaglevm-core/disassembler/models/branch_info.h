#pragma once

namespace eagle::dasm
{
    struct branch_info_t
    {
        // if false, this means that the branch could not be resolved
        // this means that the branch is stored in a register of some sort
        bool is_resolved;

        // valid branching control flow which cannot be resolved
        bool is_ret;

        // target rva of branch
        uint64_t target_rva;
    };
}
