#include <utility>

#include "eaglevm-core/virtual_machine/machines/eagle/inst_handlers.h"

#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"
#include "eaglevm-core/virtual_machine/machines/util.h"

#define VIP         regs->get_reg(inst_regs::index_vip)
#define VSP         regs->get_reg(inst_regs::index_vsp)
#define VREGS       regs->get_reg(inst_regs::index_vregs)
#define VCS         regs->get_reg(inst_regs::index_vcs)
#define VCSRET      regs->get_reg(inst_regs::index_vcsret)
#define VBASE       regs->get_reg(inst_regs::index_vbase)

#define VTEMP       regs->get_reg_temp(0)
#define VTEMP2      regs->get_reg_temp(1)
#define VTEMPX(x)   regs->get_reg_temp(x)

using namespace eagle::codec;

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

    void inst_handlers::load_register(const reg register_load, const ir::discrete_store_ptr& destination)
    {
        assert(working_block != nullptr, "working block is currently invalid");
        assert(destination->get_finalized(), "destination storage must be finalized");

        auto [status, source_reg] = handle_reg_handler_query(register_load_handlers, register_load);
        if (!status) return;

        // create a new handler
        auto& [out, label] = register_load_handlers[register_load].add_pair();
        out->bind(label);

        // find the mapped ranges required to build the register that we want
        // shuffle the ranges because we will rebuild it at random
        auto ranges_required = get_relevant_ranges(source_reg);
        std::ranges::shuffle(ranges_required, util::ran_device::get().rd);

        working_block->add(encode(m_xor, ZREG(VTEMP), ZREG(VTEMP)));

        std::vector<reg_range> stored_ranges;
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

            const reg_class dest_reg_class = get_reg_class(dest_reg);
            const reg output_reg = destination->get_store_register();

            reg temp = regs->get_reserved_temp(0);
            reg temp2 = regs->get_reserved_temp(1);

            if (dest_reg_class == gpr_64)
            {
                // gpr 64
                const std::array<std::function<void()>, 2> extractors =
                {
                    [&]
                    {
                        // bextr
                        const uint16_t length = d_to - d_from;
                        out->add(encode(m_xor, ZREG(temp), ZREG(temp2)));
                        out->add(encode(m_bextr, ZREG(temp2), dest_reg, length));
                        out->add(encode(m_shl, ZREG(temp2), s_from));
                        out->add(encode(m_and, ZREG(output_reg), ZREG(temp2)));
                    },
                    [&]
                    {
                        // shifter
                        out->add(encode(m_mov, ZREG(temp2), ZREG(dest_reg)));

                        // we only care about the data inside the rangesa d_from to d_to
                        // but we want to clear all the data outside of that
                        // to do this we do the following:

                        // 1: shift d_from to bit 0
                        out->add(encode(m_shr, ZREG(temp2), d_from));

                        // 2: shift d_to position all the way to 64th bit
                        uint16_t shift_left_orig = 64 - d_to + d_from;
                        out->add(encode(m_shl, ZREG(temp2), shift_left_orig));

                        // 3: shift again to match source position
                        uint16_t shift_to_source = 64 - s_to;
                        out->add(encode(m_shr, ZREG(temp2), shift_to_source));

                        out->add(encode(m_shr, ZREG(output_reg), ZREG(temp2)));
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

                mnemonic mnemonic = m_invalid;
                reg_class target_class = invalid;

                if (length <= 8)
                {
                    mnemonic = m_pextrb;
                    target_class = gpr_8;
                }
                else if (length <= 16)
                {
                    mnemonic = m_pextrw;
                    target_class = gpr_16;
                }
                else if (length <= 32)
                {
                    mnemonic = m_pextrd;
                    target_class = gpr_32;
                }
                else
                {
                    mnemonic = m_pextrq;
                    target_class = gpr_64;
                }

                reg target_temp = get_bit_version(temp2, target_class);
                const reg_size target_temp_size = get_reg_size(target_class);

                out->add(encode(m_xor, ZREG(temp2), ZREG(temp2)));
                out->add(encode(mnemonic, ZREG(target_temp), ZIMMS(dest_reg), ZIMMS(d_from)));

                // we have our value in VTEMP2 but it has data which we do not need
                uint16_t spillage = target_temp_size - length * 8;
                out->add(encode(m_shl, ZREG(target_temp), ZIMMS(spillage)));

                if (spillage > s_from)
                    out->add(encode(m_shr, ZREG(target_temp), ZIMMS(spillage - s_from)));
                else if (spillage < s_from)
                    out->add(encode(m_shl, ZREG(target_temp), ZIMMS(s_from - spillage)));

                out->add(encode(m_and, ZREG(output_reg), ZREG(temp2)));
            }

            stored_ranges.push_back(source_range);
        }

        call_vm_handler(label);
    }

    void inst_handlers::store_register(const reg target_reg, const ir::discrete_store_ptr& source)
    {
        assert(working_block != nullptr, "working block is currently invalid");
        assert(source->get_finalized(), "destination storage must be finalized");
        assert(source->get_store_size() == to_ir_size(get_reg_size(target_reg)), "source must be the same size as desintation register");

        auto [status, _] = handle_reg_handler_query(register_store_handlers, target_reg);
        if (!status) return;

        // create a new handler
        auto& [out, label] = register_store_handlers[target_reg].add_pair();
        out->bind(label);

        // find the mapped ranges required to build the register that we want
        // shuffle the ranges because we will rebuild it at random
        std::vector<reg_mapped_range> ranges_required = get_relevant_ranges(target_reg);
        std::ranges::shuffle(ranges_required, util::ran_device::get().rd);

        reg source_register = source->get_store_register();

        std::vector<reg_range> stored_ranges;
        for (const reg_mapped_range& mapping : ranges_required)
        {
            const auto& [source_range,dest_range, dest_reg] = mapping;

            // this is the bit ranges that will be in our source
            auto [s_from, s_to] = source_range;
            auto [d_from, d_to] = dest_range;

            const reg_class dest_reg_class = get_reg_class(dest_reg);
            if (dest_reg_class == gpr_64)
            {
                // gpr

                // read bits [s_from, s_to)
                // write those bits into [d_from, d_to)

                out->add({
                    encode(m_mov, ZREG(temp), ZREG(source_register)),
                    encode(m_mov, ZREG(temp), ZREG(source_register)),
                });
            }
            else
            {
                // xmm0

                /*
                 * step 1: setup vtemp register
                 *
                 * method 1: shift mask:
                 * mov vtmep, src
                 * shr vtemp, s_from
                 * and vtemp, (1 << (s_to - s_from)) - 1
                 *
                 * method 2: shift shift:
                 * mov vtemp, src
                 * shl vtemp, 64 - s_to
                 * shr vtemp, s_from + 64 - s_to
                 */

                reg temp = regs->get_reserved_temp(0);

                out->add({
                    encode(m_mov, temp, )
                })

                /*
                 * step 2: move into xmm
                 *
                 * pxor temp_xmm, temp_xmm
                 * movq temp_xmm, vtemp
                 */

                /*
                 * step 3: shift destination to start & clear destination
                 *
                 *
                 */

                /*
                 * step 4: or temp_xmm and destination and rotate to correct position
                 *
                 *
                 */
            }
        }
    }

    asmb::code_label_ptr inst_handlers::get_instruction_handler(const mnemonic mnemonic, const ir::x86_operand_sig& operand_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];

        ir::op_params sig = { };
        for (const ir::x86_operand& entry : operand_sig)
            sig.emplace_back(entry.operand_type, entry.operand_size);

        const std::optional<std::string> handler_id = target_mnemonic->get_handler_id(sig);
        return handler_id ? get_instruction_handler(mnemonic, handler_id.value()) : nullptr;
    }

    asmb::code_label_ptr inst_handlers::get_instruction_handler(const mnemonic mnemonic, const ir::handler_sig& handler_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];

        const std::optional<std::string> handler_id = target_mnemonic->get_handler_id(handler_sig);
        return handler_id ? get_instruction_handler(mnemonic, handler_id.value()) : nullptr;
    }

    asmb::code_label_ptr inst_handlers::get_instruction_handler(mnemonic mnemonic, std::string handler_sig)
    {
        assert(mnemonic != m_pop, "pop retreival through get_instruction_handler is blocked. use get_pop");
        assert(mnemonic != m_push, "push retreival through get_instruction_handler is blocked. use get_push");

        const std::tuple key = std::tie(mnemonic, handler_sig);
        for (const auto& [tuple, code_label] : tagged_instruction_handlers)
            if (tuple == key)
                return code_label;

        asmb::code_label_ptr label = asmb::code_label::create();
        tagged_instruction_handlers.emplace_back(key, label);

        return label;
    }

    asmb::code_label_ptr inst_handlers::get_instruction_handler(const mnemonic mnemonic, const int len, const reg_size size)
    {
        ir::handler_sig signature;
        for (auto i = 0; i < len; i++)
            signature.push_back(to_ir_size(size));

        return get_instruction_handler(mnemonic, signature);
    }

    void inst_handlers::call_vm_handler(asmb::code_label_ptr label)
    {
    }

    std::vector<reg_mapped_range> inst_handlers::get_relevant_ranges(const reg source_reg) const
    {
        const std::vector<reg_mapped_range> mappings = regs->get_register_mapped_ranges(source_reg);
        const uint16_t bits_required = get_reg_size(source_reg);

        std::vector<reg_mapped_range> ranges_required;
        for (const auto& mapping : mappings)
        {
            const auto& [source_range, _, _1] = mapping;

            const auto range_max = std::get<1>(source_range);
            if (range_max < bits_required)
                ranges_required.push_back(mapping);
        }

        return ranges_required;
    }

    std::pair<bool, reg> inst_handlers::handle_reg_handler_query(std::unordered_map<reg, tagged_handler>& handler_storage, reg reg)
    {
        if (settings->single_use_register_handlers)
        {
        ATTEMPT_CREATE_HANDLER:
            // this setting means that we want to create and reuse the same handlers
            // for loading we can just use the gpr_64 handler and scale down VTEMP

            const codec::reg bit_64_reg = get_bit_version(reg, gpr_64);
            if (!handler_storage.contains(bit_64_reg))
            {
                // this means a loader for this register does not exist, we need to create on and call it
                reg = bit_64_reg;
            }
            else
            {
                // this means that the handler exists and we can just call it
                // if it exists in "register_load_handlers" that means it has been initialized with a value
                call_vm_handler(std::get<1>(handler_storage[bit_64_reg].get_first_pair()));
                return { false, none };
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

        return { true, reg };
    }

    void inst_handlers::handle_load_register_gpr64()
    {
    }

    void inst_handlers::handle_load_register_xmm()
    {
    }
}
