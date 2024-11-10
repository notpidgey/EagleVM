#pragma once
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>
#include "eaglevm-core/codec/zydis_defs.h"

namespace eagle::virt
{
    using register_context_ptr = std::shared_ptr<class register_context>;
    class scope_register_manager
    {
    public:
        explicit scope_register_manager(register_context_ptr ctx);
        ~scope_register_manager();

        codec::reg reserve();

        template <uint8_t U>
        std::array<codec::reg, U> reserve()
        {
            std::array<codec::reg, U> arr = { };
            for (auto i = 0; i < U; i++)
                arr[i] = reserve();

            return arr;
        }

        std::vector<codec::reg> reserve_multiple(uint8_t count);

        void release(codec::reg reg);
        void release(const std::vector<codec::reg>& regs);

    private:
        register_context_ptr ctx;
        std::unordered_set<codec::reg> context_used;
    };

    class register_context : public std::enable_shared_from_this<register_context>
    {
    public:
        explicit register_context(const std::vector<codec::reg>& stores, codec::reg_class target_size);
        void reset();

        uint16_t get_available_count() const;
        std::unordered_set<codec::reg> get_all_available();

        codec::reg get_any();
        std::vector<codec::reg> get_any_multiple(uint8_t count);

        void block(codec::reg reg);
        void release(codec::reg reg);

        scope_register_manager create_scope();

    private:
        std::unordered_set<codec::reg> avaliable_stores;
        std::unordered_set<codec::reg> blocked_stores;
        codec::reg_class target_size;

        codec::reg pop_availiable_store();
    };
}
