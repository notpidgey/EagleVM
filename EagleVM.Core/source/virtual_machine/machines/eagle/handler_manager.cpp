#include <utility>
#include <ranges>

#include "eaglevm-core/virtual_machine/machines/eagle/handler_manager.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"

#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"
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
    tagged_handler_data_pair tagged_variant_handler::add_pair(reg working_reg)
    {
        auto container = asmb::code_container::create();
        auto label = asmb::code_label::create();

        const tagged_handler_data_pair pair{ container, label };
        variant_pairs.emplace_back(pair, working_reg);

        return pair;
    }

    std::pair<tagged_handler_data_pair, reg> tagged_variant_handler::get_first_pair()
    {
        return variant_pairs.front();
    }

    tagged_handler_data_pair tagged_handler::get_pair()
    {
        return data;
    }

    asmb::code_container_ptr tagged_handler::get_container()
    {
        return std::get<0>(data);
    }

    asmb::code_label_ptr tagged_handler::get_label()
    {
        return std::get<1>(data);
    }

    handler_manager::handler_manager(const machine_ptr& machine, register_manager_ptr regs, register_context_ptr regs_context, settings_ptr settings)
        : machine_inst(machine), settings(std::move(settings)), regs(std::move(regs)), regs_context(std::move(regs_context))
    {
        vm_overhead = 8 * 300;
        vm_stack_regs = 17 + 16 * 2; // we only save xmm registers on the stack
        vm_call_stack = 3;
    }

    std::pair<asmb::code_label_ptr, reg> handler_manager::load_register(const reg register_to_load, const ir::discrete_store_ptr& destination)
    {
        VM_ASSERT(destination->get_finalized(), "destination storage must be finalized");
        return load_register(register_to_load, destination->get_store_register());
    }

    std::pair<asmb::code_label_ptr, reg> handler_manager::load_register(reg register_to_load, reg load_destination)
    {
        VM_ASSERT(get_reg_class(load_destination) == gpr_64, "invalid size of load destination");
        if (settings->single_register_handlers)
        {
            register_to_load = get_bit_version(register_to_load, bit_64);
            load_destination = regs->get_reserved_temp(2);

            if (register_load_handlers.contains(register_to_load))
            {
                auto [tagged, working] = register_load_handlers[register_to_load].get_first_pair();
                return { std::get<1>(tagged), working };
            }
        }

        // create a new handler
        auto [out, label] = register_load_handlers[register_to_load].add_pair(load_destination);
        out->bind(label);

        reg target_register = load_destination;

        // find the mapped ranges required to build the register that we want
        // shuffle the ranges because we will rebuild it at random
        std::vector<reg_mapped_range> ranges_required = get_relevant_ranges(register_to_load);
        // std::ranges::shuffle(ranges_required, util::ran_device::get().gen);

        if (is_upper_8(register_to_load))
        {
            for(reg_mapped_range& mapping : ranges_required)
            {
                auto& [source_range,dest_range, dest_reg] = mapping;
                auto& [s_from, s_to] = source_range;

                s_from -= 8;
                s_to -= 8;
            }
        }

        std::array temp_regs{ regs->get_reserved_temp(0), regs->get_reserved_temp(1) };
        // std::ranges::shuffle(temp_regs, util::ran_device::get().gen);

        std::vector<reg_range> stored_ranges;
        for (const reg_mapped_range& mapping : ranges_required)
        {
            const auto& [source_range,dest_range, source_register] = mapping;
            auto [destination_start, destination_end] = source_range;
            auto [source_start, source_end] = dest_range;

            if (get_reg_class(source_register) == xmm_128)
            {
                if (source_end <= 64) // lower 64 bits of XMM
                {
                    /*
                        gpr_temp = fun_get_temps()[0];

                        movq gpr_temp, source_register // move lower 64 bits into temp
                        shl gpr_temp, 64 - source_end	// clear upper end bits
                        shr gpr_temp, 64 - source_end + source_start	// clear lower start bits
                        shl gpr_temp, destination_start	// move to intended destination location
                        or target_register, get_bit_version(gpr_temp, bit_64)	// write bits to target
                     */

                    reg gpr_temp = temp_regs[0];
                    out->add({
                        encode(m_movq, ZREG(gpr_temp), ZREG(source_register)),
                        encode(m_shl, ZREG(gpr_temp), ZIMMS(64 - source_end)),
                        encode(m_shr, ZREG(gpr_temp), ZIMMS(64 - source_end + source_start)),
                        encode(m_shl, ZREG(gpr_temp), ZIMMS(destination_start)),
                        encode(m_or, ZREG(target_register), ZREG(gpr_temp))
                    });
                }
                else if (source_start >= 64) // upper 64 bits of XMM
                {
                    /*
                        psrldq source_register, 8	// move upper 64 bits to lower 64 bits

                        source_start -= 64	// since we shifted down it will be 64 bits lower
                        source_end -= 64	// since we shifted down it will be 64 bits lower

                        gpr_temp = fun_get_temps()[0];

                        movq gpr_temp, source_register // move lower 64 bits into temp
                        shl gpr_temp, 64 - source_end	// clear upper end bits
                        shr gpr_temp, 64 - source_end + source_start	// clear lower start bits
                        shl gpr_temp, destination_start	// move to intended destination location
                        or target_register, get_bit_version(gpr_temp, bit_64)	// write bits to target

                        psrldq source_register, 8	// move lower 64 bits back to upper 64 bits
                    */


                    source_start -= 64;
                    source_end -= 64;

                    reg gpr_temp = temp_regs[0];
                    out->add({
                        encode(m_pextrq, ZREG(gpr_temp), ZREG(source_register), ZIMMS(1)),
                        encode(m_shl, ZREG(gpr_temp), ZIMMS(64 - source_end)),
                        encode(m_shr, ZREG(gpr_temp), ZIMMS(64 - source_end + source_start)),
                        encode(m_shl, ZREG(gpr_temp), ZIMMS(destination_start)),
                        encode(m_or, ZREG(target_register), ZREG(gpr_temp)),
                    });
                }
                else // cross boundary register
                {
                    /*
                        // lower boundary
                        // [source_start, 64)

                        temps = fun_get_temps(2);
                        gpr_temp = temps[0];

                        movq gpr_temp, source_register // move lower 64 bits into temp
                        shr gpr_temp, 64 - source_start	// clear lower start bits
                        shl gpr_temp, destination_start	// move to intended destination location
                        or target_register, get_bit_version(gpr_temp, bit_64)	// write bits to target

                        // upper boundary
                        // [64, source_end)

                        psrldq source_register, 8	// move upper 64 bits to lower 64 bits

                        source_start = 0	// because we read across boundaries this will now start at 0
                        source_end -= 64	// since we shifted down it will be 64 bits lower

                        gpr_temp = temps[1];

                        movq gpr_temp, source_register // move lower 64 bits into temp
                        shl gpr_temp, 64 - source_end	// clear upper end bits
                        shr gpr_temp, 64 - source_end	// move to intended destination location
                        or target_register, get_bit_version(gpr_temp, bit_64)	// write bits to target

                        psrldq source_register, 8	// move lower 64 bits back to upper 64 bits
                    */

                    // reg gpr_temp = temp_regs[0];
                    // out->add({
                    //     encode(m_movq, ZREG(gpr_temp), ZREG(source_register)),
                    //     encode(m_shr, ZREG(gpr_temp), ZIMMS(64 - source_start)),
                    //     encode(m_shl, ZREG(gpr_temp), ZIMMS(destination_start)),
                    //     encode(m_or, ZREG(target_register), ZREG(gpr_temp))
                    // });

                    // source_start = 0;
                    // source_end -= 64;

                    // gpr_temp = temp_regs[1];
                    // out->add({
                    //     encode(m_pextrq, ZREG(gpr_temp), ZREG(source_register), ZIMMS(1)),
                    //     encode(m_movq, ZREG(gpr_temp), ZREG(source_register)),
                    //     encode(m_shl, ZREG(gpr_temp), ZIMMS(64 - source_end)),
                    //     encode(m_shr, ZREG(gpr_temp), ZIMMS(64 - source_end)),
                    //     encode(m_or, ZREG(target_register), ZREG(gpr_temp)),
                    // });

                    VM_ASSERT("this should not happen as the register manager should handle cross boundary loads");
                }
            }
            else
            {
                /*
                    gpr_temp = fun_get_temps()[0];

                    mov gpr_temp, source_register // move lower 64 bits into temp
                    shl gpr_temp, 64 - source_end	// clear upper end bits
                    shr gpr_temp, 64 - source_end + source_start	// clear lower start bits
                    shl gpr_temp, destination_start	// move to intended destination location
                    or target_register, get_bit_version(gpr_temp, bit_64)	// write bits to target
                */

                reg gpr_temp = temp_regs[0];
                out->add({
                    encode(m_mov, ZREG(gpr_temp), ZREG(source_register)),
                    encode(m_shl, ZREG(gpr_temp), ZIMMS(64 - source_end)),
                    encode(m_shr, ZREG(gpr_temp), ZIMMS(64 - source_end + source_start)),
                    encode(m_shl, ZREG(gpr_temp), ZIMMS(destination_start)),
                    encode(m_or, ZREG(target_register), ZREG(gpr_temp))
                });
            }

            stored_ranges.push_back(source_range);
        }

        create_vm_return(out);
        return { label, load_destination };
    }

    std::pair<asmb::code_label_ptr, reg> handler_manager::store_register(const reg register_to_store_into, const ir::discrete_store_ptr& source)
    {
        VM_ASSERT(source->get_finalized(), "destination storage must be finalized");
        VM_ASSERT(source->get_store_size() == to_ir_size(get_reg_size(register_to_store_into)), "source must be the same size as desintation register");

        return store_register(register_to_store_into, source->get_store_register());
    }

    std::pair<asmb::code_label_ptr, reg> handler_manager::store_register(const reg register_to_store_into, reg source)
    {
        VM_ASSERT(get_reg_class(source) == gpr_64, "invalid size of load destination");

        if (settings->single_register_handlers)
        {
            if (register_store_handlers.contains(register_to_store_into))
            {
                auto [tagged, working] = register_store_handlers[register_to_store_into].get_first_pair();
                return { std::get<1>(tagged), working };
            }

            source = regs->get_reserved_temp(2);
        }

        // create a new handler
        auto [out, label] = register_store_handlers[register_to_store_into].add_pair(source);
        out->bind(label);

        // find the mapped ranges required to build the register that we want
        // shuffle the ranges because we will rebuild it at random
        std::vector<reg_mapped_range> ranges_required = get_relevant_ranges(register_to_store_into);
        // std::ranges::shuffle(ranges_required, util::ran_device::get().gen);

        if (is_upper_8(register_to_store_into))
        {
            for(reg_mapped_range& mapping : ranges_required)
            {
                auto& [source_range,dest_range, dest_reg] = mapping;
                auto& [s_from, s_to] = source_range;

                s_from -= 8;
                s_to -= 8;
            }
        }

        reg source_register = source;

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

                reg temp_xmm_q = regs->get_reserved_temp(1);
                out->add({
                    encode(m_mov, ZREG(temp_xmm_q), ZREG(dest_reg)),
                    encode(m_ror, ZREG(temp_xmm_q), ZIMMS(d_from)),
                    encode(m_shr, ZREG(temp_xmm_q), ZIMMS(bit_length)),
                    encode(m_shl, ZREG(temp_xmm_q), ZIMMS(bit_length)),
                    encode(m_or, ZREG(temp_xmm_q), ZREG(temp_value_reg)),
                    encode(m_rol, ZREG(temp_xmm_q), ZIMMS(d_from)),

                    encode(m_mov, ZREG(dest_reg), ZREG(temp_xmm_q))
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
                    reg temp_xmm_q = regs->get_reserved_temp(1);

                    const uint8_t bit_length = to - from;
                    out->add({
                        encode(m_movq, ZREG(temp_xmm_q), ZREG(dest_reg)),
                        encode(m_ror, ZREG(temp_xmm_q), ZIMMS(from)),
                        encode(m_shr, ZREG(temp_xmm_q), ZIMMS(bit_length)),
                        encode(m_shl, ZREG(temp_xmm_q), ZIMMS(bit_length)),
                        encode(m_or, ZREG(temp_xmm_q), ZREG(temp_value_reg)),
                        encode(m_rol, ZREG(temp_xmm_q), ZIMMS(from)),

                        encode(m_pinsrq, ZREG(dest_reg), ZREG(temp_xmm_q), ZIMMS(0)),
                    });
                };

                auto handle_ub = [&](auto to, auto from, auto temp_value_reg)
                {
                    reg temp_xmm_q = regs->get_reserved_temp(1);

                    const uint8_t bit_length = to - from;
                    out->add({
                        encode(m_pextrq, ZREG(temp_xmm_q), ZREG(dest_reg), ZIMMS(1)),
                        encode(m_ror, ZREG(temp_xmm_q), ZIMMS(from - 64)),
                        encode(m_shr, ZREG(temp_xmm_q), ZIMMS(bit_length)),
                        encode(m_shl, ZREG(temp_xmm_q), ZIMMS(bit_length)),
                        encode(m_or, ZREG(temp_xmm_q), ZREG(temp_value_reg)),
                        encode(m_rol, ZREG(temp_xmm_q), ZIMMS(from - 64)),

                        encode(m_pinsrq, ZREG(dest_reg), ZREG(temp_xmm_q), ZIMMS(1)),
                    });
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
                    VM_ASSERT("this should not happen, register manager needs to split cross read boundaries");
            }
        }

        create_vm_return(out);
        return { label, source_register };
    }

    asmb::code_label_ptr handler_manager::get_push(reg target_reg, reg_size size)
    {
        const reg reg = get_bit_version(target_reg, size);
        if (!vm_push.contains(reg))
            vm_push[reg] = {
                asmb::code_container::create("create_push " + std::to_string(size)),
                asmb::code_label::create("create_push " + std::to_string(size))
            };

        return std::get<1>(vm_push[reg]);
    }

    asmb::code_label_ptr handler_manager::get_pop(reg target_reg, reg_size size)
    {
        const reg reg = get_bit_version(target_reg, size);
        if (!vm_pop.contains(reg))
            vm_pop[reg] = {
                asmb::code_container::create("create_pop " + std::to_string(size)),
                asmb::code_label::create("create_pop " + std::to_string(size))
            };

        return std::get<1>(vm_pop[reg]);
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
        VM_ASSERT(mnemonic != m_pop, "pop retreival through get_instruction_handler is blocked. use get_pop");
        VM_ASSERT(mnemonic != m_push, "push retreival through get_instruction_handler is blocked. use get_push");

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

    void handler_manager::call_vm_handler(const asmb::code_container_ptr& container, const asmb::code_label_ptr& label) const
    {
        VM_ASSERT(label != nullptr, "code cannot be an invalid code label");
        const asmb::code_label_ptr return_label = asmb::code_label::create("caller return");

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        container->add(encode(m_lea, ZREG(VCS), ZMEMBD(VCS, -8, TOB(bit_64))));
        container->add(RECOMPILE(encode(m_mov, ZMEMBD(VCS, 0, TOB(bit_64)), ZLABEL(return_label))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        container->add(RECOMPILE(encode(m_mov, ZREG(VIP), ZIMMS(label->get_relative_address()))));
        container->add(RECOMPILE(encode(m_lea, ZREG(VIP), ZMEMBI(VBASE, VIP, 1, TOB(bit_64)))));
        container->add(encode(m_jmp, ZREG(VIP)));

        // execution after VM handler should end up here
        container->bind(return_label);
    }

    asmb::code_label_ptr handler_manager::get_vm_enter()
    {
        vm_enter.tag();
        return vm_enter.get_label();
    }

    asmb::code_label_ptr handler_manager::get_vm_exit()
    {
        vm_exit.tag();
        return vm_exit.get_label();
    }

    asmb::code_label_ptr handler_manager::get_rlfags_load()
    {
        vm_rflags_load.tag();
        return vm_rflags_load.get_label();
    }

    asmb::code_label_ptr handler_manager::get_rflags_store()
    {
        vm_rflags_store.tag();
        return vm_rflags_store.get_label();
    }

    reg handler_manager::get_push_working_register() const
    {
        VM_ASSERT(!settings->randomize_working_register, "can only return a working register if randomization is disabled");
        return regs->get_reserved_temp(0);
    }

    reg handler_manager::get_pop_working_register()
    {
        VM_ASSERT(!settings->randomize_working_register, "can only return a working register if randomization is disabled");
        return regs->get_reserved_temp(0);
    }

    std::vector<reg_mapped_range> handler_manager::get_relevant_ranges(const reg source_reg) const
    {
        const std::vector<reg_mapped_range> mappings = regs->get_register_mapped_ranges(get_bit_version(source_reg, bit_64));

        uint16_t min_bit = 0;
        uint16_t max_bit = get_reg_size(source_reg);

        if (is_upper_8(source_reg))
        {
            min_bit = 8;
            max_bit = 16;
        }

        std::vector<reg_mapped_range> ranges_required;
        for (const auto& mapping : mappings)
        {
            const auto& [source_range, _, _1] = mapping;

            const auto start = std::get<0>(source_range);
            const auto end = std::get<1>(source_range);
            if (start >= min_bit && end <= max_bit)
                ranges_required.push_back(mapping);
        }

        return ranges_required;
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
