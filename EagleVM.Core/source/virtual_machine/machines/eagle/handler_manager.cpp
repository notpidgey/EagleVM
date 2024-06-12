#include <utility>

#include "eaglevm-core/virtual_machine/machines/eagle/handler_manager.h"

#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"
#include "eaglevm-core/virtual_machine/machines/util.h"

#define VIP         regs->get_vm_reg(register_manager::index_vip)
#define VSP         regs->get_vm_reg(register_manager::index_vsp)
#define VREGS       regs->get_vm_reg(register_manager::index_vregs)
#define VCS         regs->get_vm_reg(register_manager::index_vcs)
#define VCSRET      regs->get_vm_reg(register_manager::index_vcsret)
#define VBASE       regs->get_vm_reg(register_manager::index_vbase)

using namespace eagle::codec;

namespace eagle::virt::eg
{
    tagged_handler_data_pair tagged_variant_handler::add_pair()
    {
        auto container = asmb::code_container::create();
        auto label = asmb::code_label::create();

        const tagged_handler_data_pair pair{ container, label };
        variant_pairs.push_back(pair);

        return pair;
    }

    tagged_handler_data_pair tagged_variant_handler::get_first_pair()
    {
        return variant_pairs.front();
    }

    handler_manager::handler_manager(machine_ptr machine, register_manager_ptr regs, machine_settings_ptr settings)
        : working_block(nullptr), machine(std::move(machine)), regs(std::move(regs)), settings(std::move(settings))
    {
        vm_overhead = 8 * 100;
        vm_stack_regs = 17;
        vm_call_stack = 3;
    }

    void handler_manager::set_working_block(const asmb::code_container_ptr& block)
    {
        working_block = block;
    }

    void handler_manager::load_register(const reg register_load, const ir::discrete_store_ptr& destination)
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

            /*
            todo: finish implementing this.
            depending on the current "reserved_ranges" we might have an opportunity to read more bits from the destination
            and write more to VTEMP without affecting

            uint16_t spillover_ceil = 64;
            uint16_t spillover_bot = 0;

            for (auto& [bot, top] : reserved_ranges)
            {
                auto check_spillover = [](
                    const reg_range source_range,
                    uint16_t& limit,
                    const uint16_t value,
                    const bool limit_greater)
                {
                    if (limit_greater && source_range.first >= value)
                    {
                        // checking ceiling
                        if (value > limit)
                            limit = value;
                    }
                    else if (source_range.second <= value)
                    {
                        // checking floor
                        if (value < limit)
                            limit = value;
                    }
                };

                check_spillover(source_range, spillover_ceil, bot, false);
                check_spillover(source_range, spillover_bot, top, true);
            }
             */

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
                const uint16_t spillage = target_temp_size - length * 8;
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

    void handler_manager::store_register(const reg target_reg, const ir::discrete_store_ptr& source)
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

                reg temp_value_reg = regs->get_reserved_temp(0);
                out->add({
                    encode(m_mov, ZREG(temp_value_reg), ZREG(source_register)),
                    encode(m_shl, ZREG(temp_value_reg), ZIMMS(64 - s_to)),
                    encode(m_shr, ZREG(temp_value_reg), ZIMMS(s_from + 64 - s_to)) // or use a mask

                });

                uint8_t bit_length = s_to - s_from;

                reg temp_xmm_q = regs->get_reserved_temp(0);
                out->add({
                    encode(m_mov, ZREG(temp_xmm_q), ZREG(dest_reg)),
                    encode(m_ror, ZREG(temp_xmm_q), ZIMMS(d_from)),
                    encode(m_shr, ZREG(temp_xmm_q), ZREG(bit_length)),
                    encode(m_shl, ZREG(temp_xmm_q), ZREG(bit_length)),
                    encode(m_or, ZREG(temp_xmm_q), ZREG(temp_value_reg)),
                    encode(m_rol, ZREG(temp_xmm_q), ZIMMS(d_from)),

                    encode(m_movq, ZREG(dest_reg), ZREG(temp_xmm_q))
                });
            }
            else
            {
                // xmm0

                /*
                 * step 1: setup vtemp register
                 *
                 * mov vtemp, src
                 * shl vtemp, 64 - s_to
                 * shr vtemp, s_from + 64 - s_to
                 */

                reg temp_value = regs->get_reserved_temp(0);
                out->add({
                    encode(m_mov, ZREG(temp_value), ZREG(source_register)),
                    encode(m_shl, ZREG(temp_value), ZIMMS(64 - s_to)),
                    encode(m_shr, ZREG(temp_value), ZIMMS(s_from + 64 - s_to)) // or use a mask
                });

                /*
                 * step 2: clear and store
                 * this is where things get complicated
                 *
                 * im pretty certain there is no SSE instruction to shift bits in an XMM register
                 * but there is one to shift bytes which really really sucks
                 */

                auto handle_lb = [&](auto to, auto from, auto temp_value_reg)
                {
                    reg temp_xmm_q = regs->get_reserved_temp(0);

                    uint8_t bit_length = to - from;
                    out->add({
                        encode(m_movq, ZREG(temp_xmm_q), ZREG(dest_reg)),
                        encode(m_ror, ZREG(temp_xmm_q), ZIMMS(d_from)),
                        encode(m_shr, ZREG(temp_xmm_q), ZREG(bit_length)),
                        encode(m_shl, ZREG(temp_xmm_q), ZREG(bit_length)),
                        encode(m_or, ZREG(temp_xmm_q), ZREG(temp_value_reg)),
                        encode(m_rol, ZREG(temp_xmm_q), ZIMMS(d_from)),

                        encode(m_movq, ZREG(dest_reg), ZREG(temp_xmm_q))
                    });
                };

                auto handle_ub = [&](auto to, auto from, auto temp_value_reg)
                {
                    from -= 64;
                    to -= 64;

                    // rotate 8 bytes so that high is in low
                    out->add(encode(m_psrldq, ZREG(dest_reg), ZIMMU(8))); // or psl

                    handle_lb(to - 64, from - 64, temp_value_reg);

                    out->add(encode(m_pslldq, ZREG(dest_reg), ZIMMU(8)));
                };

                // case 1: all the bits are located in qword 1
                // solution is simple: movq
                if (d_to <= 64)
                    handle_lb(d_to, d_from, temp_value);

                    // case 2: all bits are located in qword 2
                    // solution is simple: rotate qwords read bottom one
                else if (d_from >= 64)
                    handle_ub(d_to, d_from, temp_value);

                // case 3: we have a boundary, fuck
                else
                {
                    reg temp_value_ub = temp_value;
                    reg temp_value_lb = regs->get_reserved_temp(1);

                    out->add(encode(m_mov, ZREG(temp_value_lb), ZREG(temp_value_ub)));

                    // for ub we want to cut away the lower bits
                    out->add(encode(m_shr, ZREG(temp_value_ub), ZIMMS(64 - d_from)));

                    // for lb we want to cut away the upper bits
                    const auto left_shift_clear = 64 - (d_to - d_from) - (d_to - 64); // GPR64 - SIZE - UB_BITS
                    out->add(encode(m_shl, ZREG(temp_value_lb), ZIMMS(left_shift_clear)));
                    out->add(encode(m_shr, ZREG(temp_value_lb), ZIMMS(left_shift_clear)));

                    // handle upper boundary
                    handle_ub(d_to, 64, temp_value_ub);

                    // handle lower boundary
                    handle_lb(64, d_from, temp_value_lb);
                }
            }
        }
    }

    asmb::code_label_ptr handler_manager::get_instruction_handler(const mnemonic mnemonic, const ir::x86_operand_sig& operand_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];

        ir::op_params sig = { };
        for (const ir::x86_operand& entry : operand_sig)
            sig.emplace_back(entry.operand_type, entry.operand_size);

        const std::optional<std::string> handler_id = target_mnemonic->get_handler_id(sig);
        return handler_id ? get_instruction_handler(mnemonic, handler_id.value()) : nullptr;
    }

    asmb::code_label_ptr handler_manager::get_instruction_handler(const mnemonic mnemonic, const ir::handler_sig& handler_sig)
    {
        const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];

        const std::optional<std::string> handler_id = target_mnemonic->get_handler_id(handler_sig);
        return handler_id ? get_instruction_handler(mnemonic, handler_id.value()) : nullptr;
    }

    asmb::code_label_ptr handler_manager::get_instruction_handler(mnemonic mnemonic, std::string handler_sig)
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

    asmb::code_label_ptr handler_manager::get_instruction_handler(const mnemonic mnemonic, const int len, const reg_size size)
    {
        ir::handler_sig signature;
        for (auto i = 0; i < len; i++)
            signature.push_back(to_ir_size(size));

        return get_instruction_handler(mnemonic, signature);
    }

    void handler_manager::call_vm_handler(asmb::code_label_ptr label)
    {
    }

    asmb::code_label_ptr handler_manager::get_vm_enter()
    {
        vm_enter.tag();
        return vm_enter.get_label();
    }

    asmb::code_container_ptr handler_manager::build_vm_enter()
    {
        auto& [container, label] = vm_enter.get_pair();
        container->bind(label);

        // TODO: this is a temporary fix before i add stack overrun checks
        // we allocate the registers for the virtual machine 20 pushes after the current stack

        // reserve VM call stack
        // reserve VM stack
        container->add(encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -(8 * vm_overhead), TOB(bit_64))));

        // pushfq
        {
            container->add(encode(m_pushfq));
        }

        // push r0-r15 to stack
        regs->enumerate(
            [&container](reg reg)
            {
                if (get_reg_class(reg) == xmm_128)
                {
                    container->add({
                        encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -16, TOB(bit_64))),
                        encode(m_movdqu, ZMEMBD(rsp, 0, TOB(bit_128)), ZREG(reg))
                    });
                }
                else
                {
                    container->add(encode(m_push, ZREG(reg)));
                }
            });

        // mov VSP, rsp         ; begin virtualization by setting VSP to rsp
        // mov VREGS, VSP       ; set VREGS to currently pushed stack items
        // mov VCS, VSP         ; set VCALLSTACK to current stack top

        // lea rsp, [rsp + stack_regs + 1] ; this allows us to move the stack pointer in such a way that pushfq overwrite rflags on the stack

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead))] ; load the address of where return address is located
        // mov VTEMP, [VTEMP]   ; load actual value into VTEMP
        // lea VCS, [VCS - 8]   ; allocate space to place return address
        // mov [VCS], VTEMP     ; put return address onto call stack

        reg temp = regs->get_reserved_temp(0);
        container->add({
            encode(m_mov, ZREG(VSP), ZREG(rsp)),
            encode(m_mov, ZREG(VREGS), ZREG(VSP)),
            encode(m_mov, ZREG(VCS), ZREG(VSP)),

            encode(m_lea, ZREG(rsp), ZMEMBD(VREGS, 8 * vm_stack_regs, TOB(bit_64))),

            encode(m_lea, ZREG(temp), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead), TOB(bit_64))),
            encode(m_mov, ZREG(temp), ZMEMBD(temp, 0, TOB(bit_64))),
            encode(m_lea, ZREG(VCS), ZMEMBD(VCS, -8, TOB(bit_64))),
            encode(m_mov, ZMEMBD(VCS, 0, 8), ZREG(temp)),
        });

        // lea VIP, [0x14000000]    ; load base
        const asmb::code_label_ptr rel_label = asmb::code_label::create();
        container->bind(rel_label);
        container->add(RECOMPILE(encode(m_lea, ZREG(VBASE), ZMEMBD(rip, -rel_label->get_address(), TOB(bit_64)))));

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp (+1 because we pushed an rva)
        // mov VSP, VTEMP
        container->add({
            encode(m_lea, ZREG(temp), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead + 1), TOB(bit_64))),
            encode(m_mov, ZREG(VSP), ZREG(temp)),
        });

        create_vm_return(container);
        return container;
    }

    asmb::code_label_ptr handler_manager::get_vm_exit()
    {
        vm_exit.tag();
        return vm_exit.get_label();
    }

    asmb::code_container_ptr handler_manager::build_vm_exit()
    {
        auto& [container, label] = vm_exit.get_pair();
        container->bind(label);

        reg temp = rm->get_any();

        // we need to place the target RSP after all the pops
        // lea VTEMP, [VREGS + vm_stack_regs]
        // mov [VTEMP], VSP
        container->add({
            encode(m_lea, ZREG(temp), ZMEMBD(VREGS, 8 * vm_stack_regs, TOB(bit_64))),
            encode(m_mov, ZMEMBD(temp, 0, 8), ZREG(VSP))
        });

        // we also need to setup an RIP to return to main program execution
        // we will place that after the RSP
        const asmb::code_label_ptr rel_label = asmb::code_label::create();
        container->bind(rel_label);
        container->add(RECOMPILE(encode(m_lea, ZREG(VIP), ZMEMBD(rip, -rel_label->get_address(), TOB(bit_64)))));
        container->add(encode(m_lea, ZREG(VIP), ZMEMBI(VIP, VCSRET, 1, TOB(bit_64))));
        container->add(encode(m_mov, ZMEMBD(VSP, -8, 8), ZREG(VIP)));

        // mov rsp, VREGS
        container->add(encode(m_mov, ZREG(rsp), ZREG(VREGS)));

        //pop r0-r15 to stack
        regs->enumerate([&container](auto reg)
        {
            if (reg == ZYDIS_REGISTER_RSP || reg == ZYDIS_REGISTER_RIP)
                container->add(encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, TOB(bit_64))));
            else
            {
                if (get_reg_class(reg) == xmm_128)
                {
                    container->add({
                        encode(m_movdqu, ZREG(reg), ZMEMBD(rsp, 0, TOB(bit_128))),
                        encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 16, TOB(bit_64)))
                    });
                }
                else
                {
                    container->add(encode(m_pop, ZREG(reg)));
                }
            }
        }, true);

        //popfq
        {
            container->add(encode(m_popfq));
        }

        // the rsp that we setup earlier before popping all the regs
        container->add(encode(m_pop, ZREG(rsp)));
        container->add(encode(m_jmp, ZMEMBD(rsp, -8, TOB(bit_64))));

        return container;
    }

    asmb::code_label_ptr handler_manager::get_rlfags_load()
    {
        vm_rflags_load.tag();
        return vm_rflags_load.get_label();
    }

    asmb::code_container_ptr handler_manager::build_rflags_load()
    {
        auto& [container, label] = vm_rflags_load.get_pair();
        container->bind(label);

        container->add({
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -8, TOB(bit_64))),
            encode(m_popfq),
        });

        create_vm_return(container);
        return container;
    }

    asmb::code_label_ptr handler_manager::get_rflags_store()
    {
        vm_rflags_store.tag();
        return vm_rflags_store.get_label();
    }

    asmb::code_container_ptr handler_manager::build_rflags_store()
    {
        auto& [container, label] = vm_rflags_load.get_pair();
        container->bind(label);

        container->add({
            encode(m_pushfq),
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, TOB(bit_64))),
        });

        create_vm_return(container);
        return container;
    }

    codec::reg handler_manager::get_push_working_register()
    {
        assert(!settings->randomize_working_register, "can only return a working register if randomization is disabled");
        return regs->get_reserved_temp(0);
    }

    std::vector<asmb::code_container_ptr> handler_manager::build_push()
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (uint8_t i = 0; i < 4; i++)
        {
            tagged_handler& handler = vm_push[i];
            if (!handler.get_tagged())
                continue;

            auto& [container, label] = handler.get_pair();
            container->bind(label);

            const reg_size reg_size = load_store_index_size(i);

            reg target_temp = get_bit_version(get_push_working_register(), reg_size);
            container->add({
                encode(m_lea, ZREG(VSP), ZMEMBD(VSP, -TOB(reg_size), TOB(bit_64))),
                encode(m_mov, ZMEMBD(VSP, 0, TOB(reg_size)), ZREG(target_temp))
            });

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
    }

    codec::reg handler_manager::get_pop_working_register()
    {
        assert(!settings->randomize_working_register, "can only return a working register if randomization is disabled");
        return regs->get_reserved_temp(0);
    }

    std::vector<asmb::code_container_ptr> handler_manager::build_pop()
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (uint8_t i = 0; i < 4; i++)
        {
            tagged_handler& handler = vm_pop[i];
            if (!handler.get_tagged())
                continue;

            auto& [container, label] = handler.get_pair();
            container->bind(label);

            const reg_size reg_size = load_store_index_size(i);

            reg target_temp = get_bit_version(get_pop_working_register(), reg_size);
            container->add({
                encode(m_mov, ZREG(target_temp), ZMEMBD(VSP, 0, TOB(reg_size))),
                encode(m_lea, ZREG(VSP), ZMEMBD(VSP, TOB(reg_size), 8)),
            });

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
    }

    std::vector<reg_mapped_range> handler_manager::get_relevant_ranges(const reg source_reg) const
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

    std::pair<bool, reg> handler_manager::handle_reg_handler_query(std::unordered_map<reg, tagged_variant_handler>& handler_storage, reg reg)
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

    void handler_manager::handle_load_register_gpr64()
    {
    }

    void handler_manager::handle_load_register_xmm()
    {
    }

    void handler_manager::create_vm_return(const asmb::code_container_ptr& container) const
    {
        // mov VCSRET, [VCS]        ; pop from call stack
        // lea VCS, [VCS + 8]       ; move up the call stack pointer
        container->add(encode(m_mov, ZREG(VCSRET), ZMEMBD(VCS, 0, TOB(bit_64))));
        container->add(encode(m_lea, ZREG(VCS), ZMEMBD(VCS, 8, TOB(bit_64))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        container->add(encode(m_lea, ZREG(VIP), ZMEMBI(VBASE, VCSRET, 1, TOB(bit_64))));
        container->add(encode(m_jmp, ZREG(VIP)));
    }

    reg_size handler_manager::load_store_index_size(const uint8_t index)
    {
        switch (index)
        {
            case 0:
                return bit_64;
            case 1:
                return bit_32;
            case 2:
                return bit_16;
            case 3:
                return bit_8;
            default:
                return empty;
        }
    }
}
