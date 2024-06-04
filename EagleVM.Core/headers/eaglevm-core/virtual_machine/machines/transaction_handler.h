#pragma once
#include <unordered_set>
#include <vector>

#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"

namespace eagle::virt
{
    class transaction_handler
    {
    public:
        explicit transaction_handler(const std::vector<codec::reg>& stores);

        codec::reg assign(const ir::discrete_store_ptr& store);
        codec::reg get_any();

        void block(const ir::discrete_store_ptr& store);
        void block(codec::reg reg);
        void release(const ir::discrete_store_ptr& store);
        void release(codec::reg reg);

    private:
        std::unordered_set<codec::reg> avaliable_stores;
        std::unordered_set<codec::reg> blocked_stores;

        codec::reg pop_availiable_store();
    };

    using transaction_handler_ptr = std::shared_ptr<transaction_handler>;
}
