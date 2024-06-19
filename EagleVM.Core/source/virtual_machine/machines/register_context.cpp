#include "eaglevm-core/virtual_machine/machines/register_context.h"

#include <random>

#include "eaglevm-core/virtual_machine/machines/util.h"

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"

namespace eagle::virt
{
    register_context::register_context(const std::vector<codec::reg>& stores)
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
            return get_bit_version(store->get_store_register(), codec::gpr_64);

        const codec::reg target_register_64 = pop_availiable_store();
        block(target_register_64);

        store->finalize_register(target_register_64);
        return target_register_64;
    }

    codec::reg register_context::get_any()
    {
        VM_ASSERT(!avaliable_stores.empty(), "attempted to retreive sample from empty register storage");

        codec::reg out;
        std::ranges::sample(avaliable_stores, &out, 1, util::ran_device().get().gen);

        return out;
    }

    std::vector<codec::reg> register_context::get_any_multiple(const uint8_t count)
    {
        VM_ASSERT(avaliable_stores.size() >= count, "attempted to retreive sample from empty register storage");

        std::vector<codec::reg> out(count);
        std::ranges::sample(avaliable_stores, std::back_inserter(out), count, util::ran_device().get().gen);

        return out;
    }

    void register_context::block(const ir::discrete_store_ptr& store)
    {
        const codec::reg target_register = store->get_store_register();
        const codec::reg target_register_64 = get_bit_version(target_register, codec::gpr_64);

        VM_ASSERT(avaliable_stores.contains(target_register_64), "attempted to block unavailiable register");
        blocked_stores.insert(target_register_64);
        avaliable_stores.erase(target_register_64);
    }

    void register_context::block(const codec::reg reg)
    {
        const codec::reg target_register_64 = get_bit_version(reg, codec::gpr_64);

        VM_ASSERT(!blocked_stores.contains(target_register_64), "attempted to block blocked register");
        blocked_stores.insert(target_register_64);
        avaliable_stores.erase(target_register_64);
    }

    void register_context::release(const ir::discrete_store_ptr& store)
    {
        const codec::reg target_register = store->get_store_register();
        const codec::reg target_register_64 = get_bit_version(target_register, codec::gpr_64);

        VM_ASSERT(!avaliable_stores.contains(target_register_64), "attempted to release available register");
        avaliable_stores.insert(target_register_64);
        blocked_stores.erase(target_register_64);
    }

    void register_context::release(const codec::reg reg)
    {
        const codec::reg target_register_64 = get_bit_version(reg, codec::gpr_64);

        VM_ASSERT(blocked_stores.contains(target_register_64), "attempted to release unavailiable register");
        avaliable_stores.insert(target_register_64);
        blocked_stores.erase(target_register_64);
    }

    codec::reg register_context::pop_availiable_store()
    {
        VM_ASSERT(!avaliable_stores.empty(), "attempted to pop from empty register storage");

        const codec::reg i = get_any();
        avaliable_stores.erase(i);

        return i;
    }
}
