#include "eaglevm-core/virtual_machine/machines/register_context.h"

#include "eaglevm-core/virtual_machine/machines/util.h"

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"

namespace eagle::virt
{
    register_context::register_context(const std::vector<codec::reg>& stores, const codec::reg_class target_size)
        : target_size(target_size)
    {
        for (codec::reg store : stores)
            avaliable_stores.insert(store);
    }

    std::unordered_set<codec::reg> register_context::get_all_availiable()
    {
        return avaliable_stores;
    }

    codec::reg register_context::assign(const ir::discrete_store_ptr& store)
    {
        if (store->get_finalized())
            return get_bit_version(store->get_store_register(), target_size);

        const codec::reg target_register_64 = pop_availiable_store();
        block(target_register_64);

        store->finalize_register(target_register_64);
        return target_register_64;
    }

    codec::reg register_context::get_any()
    {
        VM_ASSERT(!avaliable_stores.empty(), "attempted to retreive sample from empty register storage");

        codec::reg out;
        std::ranges::sample(avaliable_stores, &out, 1, util::get_ran_device().gen);

        return out;
    }

    std::vector<codec::reg> register_context::get_any_multiple(const uint8_t count)
    {
        VM_ASSERT(avaliable_stores.size() >= count, "attempted to retreive sample from empty register storage");

        std::vector<codec::reg> out;
        std::ranges::sample(avaliable_stores, std::back_inserter(out), count, util::get_ran_device().gen);

        return out;
    }

    void register_context::block(const ir::discrete_store_ptr& store)
    {
        const codec::reg target_register = store->get_store_register();
        const codec::reg target_register_64 = get_bit_version(target_register, target_size);

        VM_ASSERT(avaliable_stores.contains(target_register_64), "attempted to block unavailiable register");
        blocked_stores.insert(target_register_64);
        avaliable_stores.erase(target_register_64);
    }

    void register_context::block(const codec::reg reg)
    {
        const codec::reg target_register_64 = get_bit_version(reg, target_size);

        VM_ASSERT(!blocked_stores.contains(target_register_64), "attempted to block blocked register");
        blocked_stores.insert(target_register_64);
        avaliable_stores.erase(target_register_64);
    }

    void register_context::release(const ir::discrete_store_ptr& store)
    {
        const codec::reg target_register = store->get_store_register();
        const codec::reg target_register_64 = get_bit_version(target_register, target_size);

        VM_ASSERT(!avaliable_stores.contains(target_register_64), "attempted to release available register");
        avaliable_stores.insert(target_register_64);
        blocked_stores.erase(target_register_64);
    }

    void register_context::release(const codec::reg reg)
    {
        const codec::reg target_register_64 = get_bit_version(reg, target_size);

        VM_ASSERT(blocked_stores.contains(target_register_64), "attempted to release unavailiable register");
        avaliable_stores.insert(target_register_64);
        blocked_stores.erase(target_register_64);
    }

    scope_register_manager register_context::create_scope()
    {
        return scope_register_manager(shared_from_this());
    }

    codec::reg register_context::pop_availiable_store()
    {
        VM_ASSERT(!avaliable_stores.empty(), "attempted to pop from empty register storage");

        const codec::reg i = get_any();
        avaliable_stores.erase(i);

        return i;
    }

    codec::reg scope_register_manager::reserve()
    {
        const auto any = ctx->get_any();
        ctx->block(any);

        context_used.insert(any);
        return any;
    }

    std::vector<codec::reg> scope_register_manager::reserve_multiple(const uint8_t count)
    {
        const auto any = ctx->get_any_multiple(count);
        for(const auto reg : any)
            ctx->block(reg);

        context_used.insert(any.begin(), any.end());
        return any;
    }

    void scope_register_manager::release(const codec::reg reg)
    {
        VM_ASSERT(context_used.contains(reg), "context must contain used register");
        ctx->release(reg);
        context_used.erase(reg);
    }

    void scope_register_manager::release(const std::vector<codec::reg>& regs)
    {
        for(auto reg : regs)
        {
            VM_ASSERT(context_used.contains(reg), "context must contain used register");

            ctx->release(reg);
            context_used.erase(reg);
        }
    }
}
