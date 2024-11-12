#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_type.h"

namespace eagle::virt::eg
{
    template <class T>
    void hash_combine(std::size_t& seed, const T& val)
    {
        seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    template<typename... Args>
    size_t compute_handler_hash(const ir::command_type cmd_type, const Args&... args)
    {
        size_t hash = std::hash<ir::command_type>{}(cmd_type);
        (hash_combine(hash, args), ...);

        return hash;
    }
}
