#pragma once
#include <unordered_set>
#include <vector>

#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"

namespace eagle::virt
{
    class register_context
    {
    public:
        explicit register_context(const std::vector<codec::reg>& stores);

        std::unordered_set<codec::reg> get_all_availiable();

        codec::reg assign(const ir::discrete_store_ptr& store);
        codec::reg get_any();
        std::vector<codec::reg> get_any_multiple(uint8_t count);

        void block(const ir::discrete_store_ptr& store);
        void block(codec::reg reg);
        void release(const ir::discrete_store_ptr& store);
        void release(codec::reg reg);

    private:
        std::unordered_set<codec::reg> avaliable_stores;
        std::unordered_set<codec::reg> blocked_stores;

        codec::reg pop_availiable_store();
    };

    using register_context_ptr = std::shared_ptr<register_context>;
}
