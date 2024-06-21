#pragma once
#include <unordered_set>
#include <utility>
#include <vector>

#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"

namespace eagle::virt
{
    using register_context_ptr = std::shared_ptr<class register_context>;

    class scope_register_manager;

    class register_context : public std::enable_shared_from_this<register_context>
    {
    public:
        explicit register_context(const std::vector<codec::reg>& stores, codec::reg_class target_size);

        std::unordered_set<codec::reg> get_all_availiable();

        codec::reg assign(const ir::discrete_store_ptr& store);
        codec::reg get_any();
        std::vector<codec::reg> get_any_multiple(uint8_t count);

        void block(const ir::discrete_store_ptr& store);
        void block(codec::reg reg);
        void release(const ir::discrete_store_ptr& store);
        void release(codec::reg reg);

        scope_register_manager create_scope();

    private:
        std::unordered_set<codec::reg> avaliable_stores;
        std::unordered_set<codec::reg> blocked_stores;
        codec::reg_class target_size;

        codec::reg pop_availiable_store();
    };

    class scope_register_manager
    {
    public:
        explicit scope_register_manager(register_context_ptr ctx)
            : ctx(std::move(ctx))
        {
        }

        ~scope_register_manager()
        {
            for (const auto elem : context_used)
                ctx->release(elem);
        }

        codec::reg reserve();
        std::vector<codec::reg> reserve_multiple(uint8_t count);

        void release(codec::reg reg);
        void release(const std::vector<codec::reg>& regs);

    private:
        register_context_ptr ctx;
        std::unordered_set<codec::reg> context_used;
    };
}
