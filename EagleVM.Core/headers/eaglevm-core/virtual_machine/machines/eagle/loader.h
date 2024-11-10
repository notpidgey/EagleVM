#pragma once
#include "eaglevm-core/compiler/code_container.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"

namespace eagle::virt::eg
{
    class register_loader
    {
    public:
        explicit register_loader(const register_manager_ptr& manager, const register_context_ptr& context_64, const register_context_ptr& context_128)
            : regs(manager), regs_64_context(context_64), regs_128_context(context_128)
        {
        }

        void load_register(
            codec::reg register_to_load,
            codec::reg load_destination, codec::encoder::encode_builder& out
        ) const;

        void store_register(
            codec::reg register_to_store_into,
            codec::reg source, codec::encoder::encode_builder& out
        ) const;

        static void trim_ranges(std::vector<reg_mapped_range>& ranges_required, codec::reg target);
        std::vector<reg_mapped_range> get_relevant_ranges(codec::reg source_reg) const;

    private:
        void load_register_internal(
            codec::reg load_destination,
            const std::vector<reg_mapped_range>& ranges_required,
            codec::encoder::encode_builder& out
        ) const;

        void store_register_internal(
            codec::reg source_register,
            const std::vector<reg_mapped_range>& ranges_required,
            codec::encoder::encode_builder& out
        ) const;

        register_manager_ptr regs;
        register_context_ptr regs_64_context;
        register_context_ptr regs_128_context;
    };
}
