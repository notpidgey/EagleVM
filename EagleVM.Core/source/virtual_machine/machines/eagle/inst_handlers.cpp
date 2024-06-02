#include <utility>

#include "eaglevm-core/virtual_machine/machines/eagle/inst_handlers.h"

#define VIP         regs->get_reg(inst_regs::index_vip)
#define VSP         regs->get_reg(inst_regs::index_vsp)
#define VREGS       regs->get_reg(inst_regs::index_vregs)
#define VCS         regs->get_reg(inst_regs::index_vcs)
#define VCSRET      regs->get_reg(inst_regs::index_vcsret)
#define VBASE       regs->get_reg(inst_regs::index_vbase)

#define VTEMP       regs->get_reg_temp(0)
#define VTEMP2      regs->get_reg_temp(1)
#define VTEMPX(x)   regs->get_reg_temp(x)

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

    tagged_handler_pair tagged_handler::get_first_pair()
    {
        return variant_pairs.front();
    }

    inst_handlers::inst_handlers(machine_ptr machine, inst_regs_ptr regs, settings_ptr settings)
        : working_block(nullptr), machine(std::move(machine)), regs(std::move(regs)), settings(std::move(settings))
    {
    }

    void inst_handlers::set_working_block(const asmb::code_container_ptr& block)
    {
        working_block = block;
    }

    void inst_handlers::load_register(codec::reg reg, const ir::discrete_store_ptr& destination)
    {
        assert(working_block != nullptr, "working block is currently invalid");
        assert(destination->get_finalized(), "destination storage must be finalized");
        assert(destination->get_store_size() == ir::ir_size::bit_64, "destination must be 64 bit");

        if (settings->single_use_register_handlers)
        {
        ATTEMPT_CREATE_HANDLER:
            // this setting means that we want to create and reuse the same handlers
            // for loading we can just use the gpr_64 handler and scale down VTEMP

            const codec::reg bit_64_reg = get_bit_version(reg, codec::gpr_64);
            if (!register_load_handlers.contains(bit_64_reg))
            {
                // this means a loader for this register does not exist, we need to create on and call it
                reg = bit_64_reg;
            }
            else
            {
                // this means that the handler exists and we can just call it
                // if it exists in "register_load_handlers" that means it has been initialized with a value
                call_handler(std::get<1>(register_load_handlers[bit_64_reg].get_first_pair()));
                return;
            }
        }
        else
        {
            // this means we want to attempt creating a new handler
            const float probability = settings->chance_to_generate_register_handler;
            assert(probability <= 1.0, "invalid chance selected, must be less than or equal to 1.0");

            std::uniform_real_distribution dis(0.0, 1.0);
            const bool success = util::ran_device::get().gen_dist(dis) < probability;

            if (!success)
            {
                // give our randomness, we decided not to generate a handler
                goto ATTEMPT_CREATE_HANDLER;
            }
        }

        const asmb::code_container_ptr out = asmb::code_container::create();

        const std::vector<reg_mapped_range> mappings = regs->get_register_mapped_ranges(reg);
        const uint16_t bits_required = get_reg_size(reg);

        // find the mapped ranges required to build the register that we want
        std::vector<reg_mapped_range> ranges_required;
        for (const auto& mapping : mappings)
        {
            const auto& [source_range, _, _1] = mapping;

            const auto range_max = std::get<1>(source_range);
            if (range_max < bits_required)
                ranges_required.push_back(mapping);
        }

        // shuffle the ranges because we will rebuild it at random
        std::ranges::shuffle(ranges_required, util::ran_device::get().rd);

        working_block->add(encode(codec::m_xor, ZREG(VTEMP), ZREG(VTEMP)));

        std::vector<reg_range> reserved_ranges;
        for (const reg_mapped_range& mapping : ranges_required)
        {
            const auto& [source_range,dest_range, dest_reg] = mapping;
            auto [s_from, s_to] = source_range;
            auto [d_from, d_to] = dest_range;

            // todo: finish implementing this.
            // depending on the current "reserved_ranges" we might have an opportunity to read more bits from the destination
            // and write more to VTEMP without affecting
            // uint16_t spillover_ceil = 64;
            // uint16_t spillover_bot = 0;
            //
            // for (auto& [bot, top] : reserved_ranges)
            // {
            //     auto check_spillover = [](
            //         const reg_range source_range,
            //         uint16_t& limit,
            //         const uint16_t value,
            //         const bool limit_greater)
            //     {
            //         if (limit_greater && source_range.first >= value)
            //         {
            //             // checking ceiling
            //             if (value > limit)
            //                 limit = value;
            //         }
            //         else if (source_range.second <= value)
            //         {
            //             // checking floor
            //             if (value < limit)
            //                 limit = value;
            //         }
            //     };
            //
            //     check_spillover(source_range, spillover_ceil, bot, false);
            //     check_spillover(source_range, spillover_bot, top, true);
            // }

            const codec::reg_class dest_reg_class = get_reg_class(dest_reg);

            const codec::reg output_reg = destination->get_store_register();
            if (dest_reg_class == codec::gpr_64)
            {
                // gpr 64
                const std::array<std::function<void()>, 2> extractors =
                {
                    [&]
                    {
                        // bextr
                        const uint16_t length = d_to - d_from;
                        working_block->add(encode(codec::m_xor, ZREG(VTEMP2), ZREG(VTEMP2)));
                        working_block->add(encode(codec::m_bextr, ZREG(VTEMP2), dest_reg, length));
                        working_block->add(encode(codec::m_shl, ZREG(VTEMP2), s_from));
                        working_block->add(encode(codec::m_and, ZREG(VTEMP), ZREG(VTEMP2)));
                    },
                    [&]
                    {
                        // shifter
                        working_block->add(encode(codec::m_mov, ZREG(VTEMP2), ZREG(dest_reg)));

                        // we only care about the data inside the rangesa d_from to d_to
                        // but we want to clear all the data outside of that
                        // to do this we do the following:

                        // 1: shift d_from to bit 0
                        working_block->add(encode(codec::m_shr, ZREG(VTEMP2), d_from));

                        // 2: shift d_to position all the way to 64th bit
                        uint16_t shift_left_orig = 64 - d_to + d_from;
                        working_block->add(encode(codec::m_shl, ZREG(VTEMP2), shift_left_orig));

                        // 3: shift again to match source position
                        uint16_t shift_to_source = 64 - s_to;
                        working_block->add(encode(codec::m_shr, ZREG(VTEMP2), shift_to_source));
                    }
                };

                constexpr size_t extractor_len = extractors.size();
                auto& target_fun = extractors[util::ran_device::get().gen_64() % extractor_len];

                target_fun();
            }
            else
            {
                // xmm
                // todo: complete this function
                const uint16_t length = d_to - d_from;

                codec::mnemonic mnemonic = codec::m_invalid;
                codec::reg_class target_class = codec::invalid;

                if (length <= 8)
                {
                    mnemonic = codec::m_pextrb;
                    target_class = codec::gpr_8;
                }
                else if (length <= 16)
                {
                    mnemonic = codec::m_pextrw;
                    target_class = codec::gpr_16;
                }
                else if (length <= 32)
                {
                    mnemonic = codec::m_pextrd;
                    target_class = codec::gpr_32;
                }
                else
                {
                    mnemonic = codec::m_pextrq;
                    target_class = codec::gpr_64;
                }

                codec::reg target_temp = get_bit_version(VTEMP2, target_class);
                const codec::reg_size target_temp_size = get_reg_size(target_class);

                working_block->add(encode(codec::m_xor, ZREG(VTEMP2), ZREG(VTEMP2)));
                working_block->add(encode(mnemonic, ZREG(target_temp), dest_reg, d_from));

                // we have our value in VTEMP2 but it has data which we do not need
                uint16_t spillage = target_temp_size - length * 8;
                working_block->add(encode(codec::m_shl, ZREG(target_temp), spillage));

                if (spillage > s_from)
                    working_block->add(encode(codec::m_shr, ZREG(VTEMP2), spillage - s_from));
                else if (spillage < s_from)
                    working_block->add(encode(codec::m_shl, ZREG(VTEMP2), s_from - spillage));

                working_block->add(encode(codec::m_and, ZREG(VTEMP), ZREG(VTEMP2)));
            }

            reserved_ranges.push_back(source_range);
        }

        complete_containers.push_back(out);
    }

    void inst_handlers::store_register(const codec::reg reg, const ir::discrete_store_ptr& source)
    {
        assert(working_block != nullptr, "working block is currently invalid");
        assert(source->get_finalized(), "destination storage must be finalized");

        std::vector<reg_range> mappings = regs->get_occupied_ranges(reg);

        asmb::code_container_ptr out = use_handler ? asmb::code_container::create() : working_block;
        if (use_handler)
        {
            if (settings->single_use_register_handlers)
            {
                const codec::reg bit_64_reg = get_bit_version(reg, codec::gpr_64);
                if (!register_load_handlers.contains(bit_64_reg))
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

    void inst_handlers::call_handler(asmb::code_label_ptr label)
    {
    }
}
