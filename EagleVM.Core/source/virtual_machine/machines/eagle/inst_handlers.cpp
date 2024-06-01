#include <utility>

#include "eaglevm-core/virtual_machine/machines/eagle/inst_handlers.h"

namespace eagle::virt::eg
{
    tagged_handler_pair tagged_handler::add_pair()
    {
        auto container = asmb::code_container::create();
        auto label = asmb::code_label::create();

        const tagged_handler_pair pair{ container, label };
        variant_pairs.push_back(pair);

        return pair;
    }

    inst_handlers::inst_handlers(machine_ptr machine, inst_regs_ptr regs, settings_ptr settings)
        : working_block(nullptr), machine(std::move(machine)), regs(std::move(regs)), settings(std::move(settings))
    {
    }

    void inst_handlers::set_working_block(const asmb::code_container_ptr& block)
    {
    }

    void inst_handlers::load_register(const codec::reg reg, const ir::discrete_store_ptr& destination, bool use_handler)
    {
        assert(working_block != nullptr, "working block is currently invalid");
        assert(destination->get_finalized(), "destination storage must be finalized");

        std::vector<reg_range> mappings = regs->get_occupied_ranges(reg);

        asmb::code_container_ptr out = use_handler ? asmb::code_container::create() : working_block;
        if (use_handler)
        {
            if (settings->single_use_register_handlers)
            {
                // this setting means that we want to create and reuse the same handlers
                // for loading we can just use the gpr_64 handler and scale down VTEMP

                const codec::reg bit_64_reg = get_bit_version(reg, codec::gpr_64);
                if (!register_handlers.contains(bit_64_reg))
                {
                    // this means a loader for this register does not exist, we need to create on and call it
                }
                else
                {
                    // this means that the handler exists and we can just call it
                }
            }
            else
            {

            }

            if (settings->single_use_register_handlers)
            {
            }
            else
            {
                // we want to create a new handl
            }

            complete_containers.push_back(out);
        }
        else
        {
        }
    }

    void inst_handlers::store_register(const codec::reg reg, const ir::discrete_store_ptr& source, bool use_handler)
    {
        assert(working_block != nullptr, "working block is currently invalid");
        assert(source->get_finalized(), "destination storage must be finalized");

        std::vector<reg_range> mappings = regs->get_occupied_ranges(reg);

        asmb::code_container_ptr out = use_handler ? asmb::code_container::create() : working_block;
        if (use_handler)
        {
            if (settings->single_use_register_handlers)
            {
                const codec::reg bit_64_reg = codec::get_bit_version(reg, codec::gpr_64);
                if (!register_handlers.contains(bit_64_reg))
                {
                }
            }

            if (settings->single_use_register_handlers)
            {
            }
            else
            {
                // we want to create a new handl
            }

            complete_containers.push_back(out);
        }
        else
        {
        }
    }
}
