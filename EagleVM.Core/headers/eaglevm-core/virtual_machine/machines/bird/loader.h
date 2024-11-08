#pragma once
#include "eaglevm-core/compiler/code_container.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"

namespace eagle::virt::eg
{
    class register_loader
    {
        std::pair<asmb::code_label_ptr, codec::reg> load_register(
            codec::reg register_to_load,
            codec::reg load_destination
        );

        void load_register_internal(
            codec::reg load_destination,
            const asmb::code_container_ptr& out,
            const std::vector<reg_mapped_range>& ranges_required
        ) const;

        std::pair<asmb::code_label_ptr, codec::reg> store_register(
            codec::reg register_to_store_into,
            codec::reg source
        );

        void store_register_internal(
            codec::reg source_register,
            const asmb::code_container_ptr& out,
            const std::vector<reg_mapped_range>& ranges_required
        ) const;

        static void trim_ranges(std::vector<reg_mapped_range>& ranges_required, const codec::reg target);
        std::vector<reg_mapped_range> get_relevant_ranges(const codec::reg source_reg) const;
    };
}
