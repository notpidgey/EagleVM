#include "eaglevm-core/virtual_machine/machines/transaction_handler.h"

#include <random>

#include "eaglevm-core/virtual_machine/machines/util.h"

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"

namespace eagle::virt
{
    transaction_handler::transaction_handler(const std::vector<codec::reg>& stores)
    {
        for (codec::reg store : stores)
            avaliable_stores.insert(store);
    }

    codec::reg transaction_handler::assign(const ir::discrete_store_ptr& store)
    {
        if (store->get_finalized())
            return get_bit_version(store->get_store_register(), codec::gpr_64);

        const codec::reg target_register_64 = pop_availiable_store();
        block(target_register_64);

        const codec::reg_class target_class = get_gpr_class_from_size(to_reg_size(store->get_store_size()));

        const codec::reg target_register = get_bit_version(target_register_64, target_class);
        store->finalize_register(target_register);

        return target_register;
    }

    codec::reg transaction_handler::get_any()
    {
        codec::reg out;
        std::ranges::sample(avaliable_stores, &out, 1, util::ran_device().get().gen);

        return out;
    }

    std::vector<codec::reg> transaction_handler::get_any_multiple(const uint8_t count)
    {
        std::vector<codec::reg> out(count);
        std::ranges::sample(avaliable_stores, std::back_inserter(out), count, util::ran_device().get().gen);

        return out;
    }

    void transaction_handler::block(const ir::discrete_store_ptr& store)
    {
        const codec::reg target_register = store->get_store_register();
        const codec::reg target_register_64 = get_bit_version(target_register, codec::gpr_64);

        assert(avaliable_stores.contains(target_register_64), "attempted to block unavailiable register");
        blocked_stores.insert(target_register_64);
        avaliable_stores.erase(target_register_64);
    }

    void transaction_handler::block(const codec::reg reg)
    {
        const codec::reg target_register_64 = get_bit_version(reg, codec::gpr_64);

        assert(avaliable_stores.contains(target_register_64), "attempted to block unavailiable register");
        blocked_stores.insert(target_register_64);
        avaliable_stores.erase(target_register_64);
    }

    void transaction_handler::release(const ir::discrete_store_ptr& store)
    {
        const codec::reg target_register = store->get_store_register();
        const codec::reg target_register_64 = get_bit_version(target_register, codec::gpr_64);

        assert(blocked_stores.contains(target_register_64), "attempted to release unavailiable register");
        avaliable_stores.insert(target_register_64);
        blocked_stores.erase(target_register_64);
    }

    void transaction_handler::release(const codec::reg reg)
    {
        const codec::reg target_register_64 = get_bit_version(reg, codec::gpr_64);

        assert(blocked_stores.contains(target_register_64), "attempted to release unavailiable register");
        avaliable_stores.insert(target_register_64);
        blocked_stores.erase(target_register_64);
    }

    codec::reg transaction_handler::pop_availiable_store()
    {
        assert(!avaliable_stores.empty(), "attempted to pop from empty register storage");

        const codec::reg i = get_any();
        avaliable_stores.erase(i);

        return i;
    }
}
