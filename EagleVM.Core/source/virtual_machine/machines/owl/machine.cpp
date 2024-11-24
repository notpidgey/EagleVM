#include "eaglevm-core/virtual_machine/machines/owl/machine.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"
#include "eaglevm-core/virtual_machine/machines/owl/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/owl/settings.h"

#include <unordered_set>

#include "eaglevm-core/virtual_machine/machines/util.h"
#include "eaglevm-core/virtual_machine/machines/owl/handler.h"
#include "eaglevm-core/virtual_machine/machines/owl/loader.h"
#include "eaglevm-core/virtual_machine/machines/owl/stack.h"

#define VIP regs->get_vm_reg(register_manager::index_vip)
#define VSP regs->get_vm_reg(register_manager::index_vsp)
#define VREGS regs->get_vm_reg(register_manager::index_vregs)
#define VCS regs->get_vm_reg(register_manager::index_vcs)
#define VCSRET regs->get_vm_reg(register_manager::index_vcsret)
#define VBASE regs->get_vm_reg(register_manager::index_vbase)
#define VFLAGS regs->get_vm_reg(register_manager::index_vflags)

#define VTEMP regs->get_reserved_temp(0)
#define VTEMP2 regs->get_reserved_temp(1)
#define VTEMPX(x) regs->get_reserved_temp(x)

namespace eagle::virt::owl
{
    using namespace codec;
    using namespace codec::encoder;

    machine::machine(const settings_ptr& settings_info)
    {
        settings = settings_info;
    }

    machine_ptr machine::create(const settings_ptr& settings_info)
    {
        const std::shared_ptr<machine> instance = std::make_shared<machine>(settings_info);
        const std::shared_ptr<register_manager> reg_man = std::make_shared<register_manager>(settings_info);
        reg_man->init_reg_order();
        reg_man->create_mappings();

        const std::shared_ptr<register_context> reg_ctx_64 = std::make_shared<register_context>(reg_man->get_unreserved_temp(), codec::gpr_64);
        const std::shared_ptr<register_context> reg_ctx_128 = std::make_shared<register_context>(reg_man->get_unreserved_temp_xmm(), codec::xmm_128);
        // const std::shared_ptr<handler_manager> han_man = std::make_shared<handler_manager>(instance, reg_man, reg_ctx_64, reg_ctx_128, settings_info);

        instance->regs = reg_man;
        instance->reg_64_container = reg_ctx_64;
        instance->reg_128_container = reg_ctx_128;
        // instance->han_man = han_man;

        return instance;
    }

    asmb::code_container_ptr machine::lift_block(const ir::block_ptr& block)
    {
        const size_t command_count = block->size();
        const asmb::code_container_ptr code = asmb::code_container::create(
            std::format("block 0x{:x}, {} inst", block->block_id, command_count),
            true
        );

        if (block_context.contains(block))
        {
            const asmb::code_label_ptr label = block_context[block];
            code->label(label);
        }

        for (size_t i = 0; i < command_count; i++)
        {
            const ir::base_command_ptr command = block->at(i);
            dispatch_handle_cmd(code, command);
        }

        // TODO add checks that these were actually freed?
        reg_64_container->reset();
        reg_128_container->reset();

        return code;
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_load_ptr& cmd)
    {
        const auto load_reg = cmd->get_reg();
        if (get_reg_class(load_reg) == seg)
        {
            create_handler(default_create, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;
                    push_64(load_reg, out); // segments are going to be preserved
                }, load_reg);
        }
        else
        {
            create_handler(default_create, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;
                    stack_loader stack(out, alloc_reg);

                    auto [ymm_target, ymm_qword_lane] = regs->get_gpr_map(load_reg);
                    const reg temp_ymm = alloc_reg(ymm_256);
                    out
                        .make(m_vmovdqu64, reg_op(temp_ymm), reg_op(ymm_target))
                        .make(m_vpshrdq, reg_op(temp_ymm), imm_op(ymm_qword_lane));

                    if (load_reg == ah || load_reg == bh || load_reg == ch || load_reg == dh)
                    {
                        // ... why IS THERE NO VPERMB ON AVX2??????
                        const auto temp_xmm = get_bit_version(temp_ymm, bit_128);
                        const auto temp_gpr = alloc_reg(gpr_64);

                        // upper reg means we have to shuffle to the lowest byte of xmm reg
                        out
                            .make(m_mov, reg_op(temp_gpr), imm_op(1))
                            .make(m_pshufb, reg_op(temp_xmm), reg_op(temp_gpr));
                    }

                    // all stack operations are bit 64 so it actually doesnt matter : )
                    stack.push_gpr(temp_ymm);
                }, load_reg);
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_store_ptr& cmd)
    {
        const auto store_reg = cmd->get_reg();
        const auto size = get_reg_size(store_reg);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            auto [ymm_target, ymm_qword_lane] = regs->get_gpr_map(store_reg);

            VM_ASSERT(store_reg != ah && store_reg != bh && store_reg != ch && store_reg != dh, "not implemented for now");
            stack.pop_to_ymm(ymm_target, ymm_qword_lane, size);
        }, store_reg);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_branch_ptr& cmd)
    {
        ir::exit_condition condition = cmd->get_condition();
        std::vector<ir::ir_exit_result> push_order =
            condition != ir::exit_condition::jmp
                ? std::vector{ cmd->get_condition_special(), cmd->get_condition_default() }
                : std::vector{ cmd->get_condition_default() };

        // if the condition is inverted, we want the opposite branches to be taken
        if (condition != ir::exit_condition::jmp && cmd->is_inverted())
            std::swap(push_order[0], push_order[1]);

        // todo: dont even want to think about this
        if (cmd->is_virtual())
        {
            create_handler(force_inline, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;

                    // push all
                    for (const ir::ir_exit_result& exit_result : push_order)
                    {
                        std::visit([&](auto&& arg)
                        {
                            const reg temp_reg = alloc_reg();

                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, uint64_t>)
                            {
                                const uint64_t immediate_value = arg;

                                out.make(m_lea, reg_op(temp_reg), mem_op(VBASE, immediate_value, 8))
                                   .make(m_sub, reg_op(VSP), imm_op(bit_64))
                                   .make(m_mov, mem_op(VSP, 0, bit_64), reg_op(temp_reg));
                            }
                            else if constexpr (std::is_same_v<T, ir::block_ptr>)
                            {
                                const ir::block_ptr target = arg;

                                const asmb::code_label_ptr label = get_block_label(target);
                                VM_ASSERT(label != nullptr, "block must not be pointing to null label, missing context");

                                out.make(m_mov, reg_op(temp_reg), reg_op(VBASE))
                                   .make(m_add, reg_op(temp_reg), imm_label_operand(label))
                                   .make(m_sub, reg_op(VSP), imm_op(bit_64))
                                   .make(m_mov, mem_op(VSP, 0, bit_64), reg_op(temp_reg));
                            }
                            else
                            VM_ASSERT("unimplemented exit result");
                        }, exit_result);
                    }

                    // this is a hacky solution but the exit condition maps to an actual handler id.
                    // so we can just cast the exit_condition to a uint64_t and that will give us the handler id :)
                    // we also want to inline this so we are inserting it in place
                    const ir::ir_insts jcc_instructions = handler_manager::generate_handler(m_jmp, static_cast<uint64_t>(condition));
                    for (auto& inst : jcc_instructions)
                        dispatch_handle_cmd(block, inst);
                });
        }
        else
        {
            create_handler(force_inline, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;

                    auto jmp_non_virtual = [&](ir::ir_exit_result result)
                    {
                        return std::visit([&]<typename result>(result&& arg)
                        {
                            using T = std::decay_t<result>;
                            if constexpr (std::is_same_v<T, uint64_t>)
                            {
                                const uint64_t immediate_value = arg;
                                out.make(m_jmp, imm_op(immediate_value, true));
                            }
                            else if constexpr (std::is_same_v<T, ir::block_ptr>)
                            {
                                const ir::block_ptr target = arg;

                                const asmb::code_label_ptr label = get_block_label(target);
                                VM_ASSERT(label != nullptr, "block must not be pointing to null label, missing context");

                                out.make(m_jmp, imm_label_operand(label, true));
                            }
                            else
                            VM_ASSERT("unimplemented exit result");
                        }, result);
                    };

                    if (condition != ir::exit_condition::jmp)
                    {
                        // the inverted (second) mnemonic is useless for now, but im going to keep it here anyways
                        static const std::unordered_map<ir::exit_condition, std::array<mnemonic, 2>> jcc_lookup =
                        {
                            { ir::exit_condition::jo, { m_jo, m_jno } },
                            { ir::exit_condition::js, { m_js, m_jns } },
                            { ir::exit_condition::je, { m_jz, m_jnz } },
                            { ir::exit_condition::jb, { m_jb, m_jnb } },
                            { ir::exit_condition::jbe, { m_jbe, m_jnbe } },
                            { ir::exit_condition::jl, { m_jl, m_jnl } },
                            { ir::exit_condition::jle, { m_jle, m_jnle } },
                            { ir::exit_condition::jp, { m_jp, m_jnp } },

                            { ir::exit_condition::jcxz, { m_jcxz, m_invalid } },
                            { ir::exit_condition::jecxz, { m_jecxz, m_invalid } },
                            { ir::exit_condition::jrcxz, { m_jrcxz, m_invalid } },

                            { ir::exit_condition::jmp, { m_jmp, m_invalid } }
                        };

                        const asmb::code_label_ptr special_label = asmb::code_label::create();
                        out.make(jcc_lookup.at(condition)[0], imm_label_operand(special_label, true));

                        // normal condition
                        jmp_non_virtual(push_order[0]);

                        // special condition
                        out.label(special_label);
                        jmp_non_virtual(push_order[1]);
                    }
                    else jmp_non_virtual(push_order[0]);
                });
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_handler_call_ptr& cmd)
    {
        const auto mnemonic = cmd->get_mnemonic();

        std::vector<ir::base_command_ptr> generated_instructions;
        if (cmd->is_operand_sig())
        {
            const ir::x86_operand_sig sig = cmd->get_x86_signature();
            generated_instructions = handler_manager::generate_handler(mnemonic, sig);
        }
        else
        {
            const ir::handler_sig sig = cmd->get_handler_signature();
            generated_instructions = handler_manager::generate_handler(mnemonic, sig);
        }

        const asmb::code_container_ptr container = asmb::code_container::create();
        const auto target_label = asmb::code_label::create();
        container->bind_start(target_label);

        for (const ir::base_command_ptr& instruction : generated_instructions)
            dispatch_handle_cmd(container, instruction);

        container->make(m_mov, reg_op(VCSRET), mem_op(VCS, 0, bit_64))
                 .make(m_lea, reg_op(VCS), mem_op(VCS, 8, bit_64))
                 .make(m_lea, reg_op(VIP), mem_op(VBASE, VCSRET, 1, 0, bit_64))
                 .make(m_jmp, reg_op(VIP));

        // write the call into the current block
        const asmb::code_label_ptr return_label = asmb::code_label::create();
        encode_builder& out = *block;

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        out.make(m_lea, reg_op(VCS), mem_op(VCS, -8, bit_64))
           .make(m_mov, mem_op(VCS, 0, bit_64), imm_label_operand(return_label));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        out.make(m_mov, reg_op(VIP), imm_label_operand(target_label))
           .make(m_lea, reg_op(VIP), mem_op(VBASE, VIP, 1, 0, bit_64))
           .make(m_jmp, reg_op(VIP));

        // execution after VM handler should end up here
        out.label(return_label);

        misc_handlers.emplace_back(target_label, container);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_read_ptr& cmd)
    {
        const ir::ir_size value_size = cmd->get_read_size();
        const auto value_reg_size = to_reg_size(value_size);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            const auto [ymm_out, gpr_lane] = stack.pop_64(true);
            move_qlane_to_bottom(ymm_out, gpr_lane, out);
            read_qlane_to_gpr(ymm_out, gpr_lane, out);

            const auto value_reg = get_bit_version(gpr_lane, value_reg_size);
            out.make(m_mov, reg_op(value_reg), mem_op(gpr_lane, 0, value_reg_size));

            stack.push_gpr(gpr_lane);
        }, value_size);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_write_ptr& cmd)
    {
        // TODO: there should not be a difference between value and write size... remove this
        const ir::ir_size value_size = cmd->get_value_size();
        const auto value_reg_size = to_reg_size(value_size);

        if (const auto nearest = cmd->get_is_value_nearest())
        {
            create_handler(default_create, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;
                    stack_loader stack(out, alloc_reg);

                    auto& [ymm_value, gpr_value_lane] = stack.pop_64();
                    auto& [ymm_mem, gpr_mem_lane] = stack.pop_64();

                    read_qlane_to_gpr(ymm_value, gpr_value_lane, out);
                    read_qlane_to_gpr(ymm_mem, gpr_mem_lane, out);

                    out.make(m_mov, mem_op(gpr_mem_lane, 0, value_reg_size), reg_op(get_bit_version(gpr_value_lane, value_reg_size)));
                }, nearest, value_reg_size);
        }
        else
        {
            create_handler(default_create, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;
                    stack_loader stack(out, alloc_reg);

                    auto& [ymm_mem, gpr_mem_lane] = stack.pop_64();
                    auto& [ymm_value, gpr_value_lane] = stack.pop_64();

                    read_qlane_to_gpr(ymm_value, gpr_value_lane, out);
                    read_qlane_to_gpr(ymm_mem, gpr_mem_lane, out);

                    out.make(m_mov, mem_op(gpr_mem_lane, 0, value_reg_size), reg_op(get_bit_version(gpr_value_lane, value_reg_size)));
                }, nearest, value_reg_size);
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_pop_ptr& cmd)
    {
        const auto pop_size = cmd->get_size();
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            stack.pop_64();
        }, pop_size);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_push_ptr& cmd)
    {
        auto push_value = cmd->get_value();
        auto push_size = cmd->get_size();
        auto push_reg_size = to_reg_size(push_size);

        std::visit([&]<typename push_type>(push_type&& arg)
        {
            using T = std::decay_t<push_type>;
            if constexpr (std::is_same_v<T, uint64_t>)
            {
                const uint64_t immediate_value = arg;

                create_handler(force_inline, block, cmd,
                    [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                    {
                        encode_builder& out = *out_container;
                        stack_loader stack(out, alloc_reg);

                        const auto ymm_temp = alloc_reg(ymm_256);
                        const auto gpr_temp = alloc_reg(gpr_64);

                        out
                            .make(m_mov, reg_op(gpr_temp), imm_op(immediate_value))
                            .make(m_vpbroadcastq, reg_op(ymm_temp), reg_op(gpr_temp));

                        if (settings->randomize_push_lane)
                            stack.push_ymm_lane(ymm_temp, util::get_ran_device().gen_8() % 4);
                        else
                            stack.push_ymm_lane(ymm_temp, 0);
                    });
            }
            else if constexpr (std::is_same_v<T, ir::block_ptr>)
            {
                const ir::block_ptr& target = arg;
                const asmb::code_label_ptr label = get_block_label(target);
                VM_ASSERT(label != nullptr, "block contains missing context");

                create_handler(force_inline, block, cmd,
                    [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                    {
                        encode_builder& out = *out_container;
                        stack_loader stack(out, alloc_reg);

                        const auto ymm_temp = alloc_reg(ymm_256);
                        const auto gpr_temp = alloc_reg(gpr_64);

                        out
                            .make(m_mov, reg_op(gpr_temp), imm_label_operand(label))
                            .make(m_vpbroadcastq, reg_op(ymm_temp), reg_op(gpr_temp));

                        if (settings->randomize_push_lane)
                            stack.push_ymm_lane(ymm_temp, util::get_ran_device().gen_8() % 4);
                        else
                            stack.push_ymm_lane(ymm_temp, 0);
                    });
            }
            else
            VM_ASSERT("deprecated command type");
        }, push_value);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_load_ptr& cmd)
    {
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            const auto gpr_temp = alloc_reg(gpr_64);
            out.make(m_mov, reg_op(gpr_temp), mem_op(rsp, -8, bit_64));
            stack.push_gpr(gpr_temp);
        });
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_store_ptr& cmd)
    {
        const ir::x86_cpu_flag flags = cmd->get_relevant_flags();

        // push flags value as 64 bit
        handle_cmd(block, std::make_shared<ir::cmd_push>(flags, ir::ir_size::bit_64));

        // we can treat the actual context store as a generic handler because we got rid of the flags store
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            const auto ymm_temp = alloc_reg(ymm_256);
            const auto xmm_temp = get_bit_version(ymm_temp, xmm_128);

            auto [ymm_mask, gpr_mask_lane] = stack.pop_64();
            auto [ymm_flag, gpr_flag_lane] = stack.pop_64();

            move_qlane_to_bottom(ymm_mask, gpr_mask_lane, out);
            move_qlane_to_bottom(ymm_flag, gpr_flag_lane, out);

            // todo: instead of moving to lowest lane, have an option to randomly gen a lane
            // this way, the push_ymm_lane can use that lane

            out
                // mask off the wanted bits from flags
                .make(m_vpandq, reg_op(ymm_flag), reg_op(ymm_mask))

                // flip the mask, and remove unwanted bits from destination
                .make(m_vpcmpeqd, reg_op(ymm_temp), reg_op(ymm_temp), reg_op(ymm_temp))
                .make(m_vpxor, reg_op(ymm_mask), reg_op(ymm_mask), reg_op(ymm_temp))
                .make(m_vpandq, reg_op(ymm_flag), reg_op(ymm_mask))

                // load rflags
                // combine
                .make(m_vpbroadcastq, reg_op(ymm_temp), mem_op(rsp, -8, bit_64))
                .make(m_vpandq, reg_op(ymm_temp), reg_op(ymm_mask))
                .make(m_vporq, reg_op(ymm_temp), reg_op(ymm_flag))

                // write to rflags
                .make(m_vmovq, mem_op(rsp, -8, 0), reg_op(xmm_temp));

            stack.push_ymm_lane(ymm_flag, 0);
        });
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sx_ptr& cmd)
    {
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            const auto from_size = to_reg_size(cmd->get_current());
            const auto to_size = to_reg_size(cmd->get_target());

            mnemonic target = m_invalid;
            switch (from_size)
            {
                case bit_32:
                {
                    switch (to_size)
                    {
                        case bit_64:
                            target = m_vpmovsxdq;
                            break;
                    }
                    break;
                }
                case bit_16:
                {
                    switch (to_size)
                    {
                        case bit_64:
                            target = m_vpmovsxwq;
                            break;
                        case bit_32:
                            target = m_vpmovsxwd;
                            break;
                    }
                    break;
                }
                case bit_8:
                {
                    switch (to_size)
                    {
                        case bit_64:
                            target = m_vpmovsxbq;
                            break;
                        case bit_32:
                            target = m_vpmovsxbd;
                            break;
                        case bit_16:
                            target = m_vpmovsxbw;
                            break;
                    }
                    break;
                }
            }
            VM_ASSERT(target != m_invalid, "invalid sx instruction");

            auto [ymm_out, gpr_lane] = stack.pop_64();

            move_qlane_to_bottom(ymm_out, gpr_lane, out);
            out.make(target, reg_op(ymm_out), reg_op(get_bit_version(ymm_out, bit_128)));

            stack.push_ymm_lane(ymm_out, 0);
        }, cmd->get_current(), cmd->get_target());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd)
    {
        block->add(cmd->get_request());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_flags_load_ptr& cmd)
    {
        const reg vflag_reg = regs->get_vm_reg(register_manager::index_vflags);
        const uint32_t flag_index = ir::cmd_flags_load::get_flag_index(cmd->get_flag());

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            stack_loader stack(out, alloc_reg);

            const reg flag_hold = alloc_reg(ymm_256);
            const reg gpr_temp = alloc_reg(gpr_64);

            // todo: this is super shit. fix thsi
            out
                // load flag reg. doesnt have to be a broadcast but it will make for some nicer obf passes
                .make(m_vpbroadcastq, reg_op(flag_hold), reg_op(vflag_reg))
                .make(m_vpshrdq, reg_op(flag_hold), imm_op(flag_index)) // todo: vpshrdq not supported
                .make(m_vpand, reg_op(flag_hold), imm_op(1)); // todo: invalid

            stack.push_ymm_lane(flag_hold, 0);
        }, flag_index);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_jmp_ptr& cmd)
    {
        create_handler(force_inline, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const reg pop_reg = alloc_reg();
            out.make(m_mov, reg_op(pop_reg), mem_op(VSP, 0, bit_64))
               .make(m_add, reg_op(VSP), imm_op(bit_64))
               .make(m_jmp, reg_op(pop_reg));
        });
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_and_ptr& cmd)
    {
        // TODO: handle reversed parameters

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            handle_generic_logic_cmd(m_and, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_or_ptr& cmd)
    {
        // TODO: handle reversed parameters

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            handle_generic_logic_cmd(m_or, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_xor_ptr& cmd)
    {
        // TODO: handle reversed parameters

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            handle_generic_logic_cmd(m_xor, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shl_ptr& cmd)
    {
        // TODO: handle reversed parameters

        const reg_size size = to_reg_size(cmd->get_size());
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const auto value_reg = alloc_reg();
            const auto value_reg_size = get_bit_version(value_reg, size);
            const auto shift_reg = alloc_reg();
            const auto shift_reg_size = get_bit_version(shift_reg, size);

            out.make(m_mov, reg_op(shift_reg_size), mem_op(VSP, 0, size));
            out.make(m_mov, reg_op(value_reg_size), mem_op(VSP, TOB(size), size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));
            else
                out.make(m_add, reg_op(VSP), imm_op(size));

            if (cmd->get_size() < ir::ir_size::bit_64)
            {
                if (cmd->get_size() != ir::ir_size::bit_32)
                {
                    const reg from = get_bit_version(value_reg_size, to_reg_size(cmd->get_size()));
                    const reg to = get_bit_version(value_reg_size, to_reg_size(ir::ir_size::bit_64));

                    block->make(m_movzx, reg_op(to), reg_op(from));
                }
            }

            out.make(m_shlx, reg_op(value_reg), reg_op(value_reg), reg_op(shift_reg))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(value_reg_size));
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shr_ptr& cmd)
    {
        // TODO: handle reversed parameters

        const reg_size size = to_reg_size(cmd->get_size());
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const auto value_reg = alloc_reg();
            const auto value_reg_size = get_bit_version(value_reg, size);
            const auto shift_reg = alloc_reg();
            const auto shift_reg_size = get_bit_version(shift_reg, size);

            out.make(m_mov, reg_op(shift_reg_size), mem_op(VSP, 0, size));
            out.make(m_mov, reg_op(value_reg_size), mem_op(VSP, TOB(size), size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));
            else
                out.make(m_add, reg_op(VSP), imm_op(size));

            if (cmd->get_size() < ir::ir_size::bit_64)
            {
                if (cmd->get_size() != ir::ir_size::bit_32)
                {
                    const reg from = get_bit_version(value_reg_size, to_reg_size(cmd->get_size()));
                    const reg to = get_bit_version(value_reg_size, to_reg_size(ir::ir_size::bit_64));

                    block->make(m_movzx, reg_op(to), reg_op(from));
                }
            }

            out.make(m_shrx, reg_op(value_reg), reg_op(value_reg), reg_op(shift_reg))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(value_reg_size));
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_add_ptr& cmd)
    {
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            handle_generic_logic_cmd(m_add, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sub_ptr& cmd)
    {
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;
            handle_generic_logic_cmd(m_sub, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        }, cmd->get_reversed(), cmd->get_preserved(), cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cmp_ptr& cmd)
    {
        const reg_size size = to_reg_size(cmd->get_size());
        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            // im not even going to bother naming these, idfc read the assembly
            auto reg_zero = alloc_reg();
            auto reg_one = alloc_reg();
            const auto reg_two = alloc_reg();
            const auto reg_three = alloc_reg();

            out.make(m_xor, reg_op(reg_zero), reg_op(reg_zero))
               .make(m_xor, reg_op(reg_one), reg_op(reg_one));

            reg_zero = get_bit_version(reg_zero, size);
            reg_one = get_bit_version(reg_one, size);

            out.make(m_mov, reg_op(reg_zero), mem_op(VSP, 0, size))
               .make(m_add, reg_op(VSP), imm_op(size));

            out.make(m_mov, reg_op(reg_one), mem_op(VSP, 0, size))
               .make(m_add, reg_op(VSP), imm_op(size));

            for (const ir::vm_flags flag : ir::vm_flags_list)
            {
                const uint8_t idx = ir::cmd_flags_load::get_flag_index(flag);
                auto build_flag_load = [&](const mnemonic mnemonic)
                {
                    out.make(m_mov, reg_op(reg_two), imm_op(~(1ull << idx)))
                       .make(m_and, reg_op(VFLAGS), reg_op((reg_two)))

                       .make(m_xor, reg_op(reg_two), reg_op(reg_two))
                       .make(m_cmp, reg_op(reg_zero), reg_op(reg_one))
                       .make(m_mov, reg_op(reg_three), imm_op(1ull << idx))
                       .make(mnemonic, reg_op(reg_two), reg_op(reg_three))
                       .make(m_or, reg_op(VFLAGS), reg_op(reg_two));
                };

                switch (flag)
                {
                    case ir::vm_flags::eq:
                    {
                        // set eq = zf
                        build_flag_load(m_cmovz);
                        break;
                    }
                    case ir::vm_flags::le:
                    {
                        // set le = SF <> OF
                        build_flag_load(m_cmovl);
                        break;
                    }
                    case ir::vm_flags::ge:
                    {
                        // set ge = ZF = 0 and SF = OF
                        build_flag_load(m_cmovnl);
                        break;
                    }
                }
            }
        }, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_resize_ptr& cmd)
    {
        const reg_size from_size = to_reg_size(cmd->get_current());
        const reg_size target_size = to_reg_size(cmd->get_target());

        if (target_size > from_size)
        {
            const auto bit_diff = static_cast<uint32_t>(target_size) - static_cast<uint32_t>(from_size);
            create_handler(default_create, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;

                    const auto target_reg = alloc_reg();
                    const auto pop_reg_size = get_bit_version(target_reg, from_size);
                    const auto target_reg_size = get_bit_version(target_reg, target_size);

                    out.make(m_xor, reg_op(target_reg), reg_op(target_reg))
                       .make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, from_size))
                       .make(m_sub, reg_op(VSP), imm_op(bit_diff / 8))
                       .make(m_mov, mem_op(VSP, 0, target_size), reg_op(target_reg_size));
                }, cmd->get_current(), cmd->get_target());
        }
        else if (from_size > target_size)
        {
            const auto bit_diff = static_cast<uint32_t>(from_size) - static_cast<uint32_t>(target_size);
            create_handler(default_create, block, cmd,
                [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
                {
                    encode_builder& out = *out_container;

                    const auto target_reg = alloc_reg();
                    const auto pop_reg_size = get_bit_version(target_reg, from_size);
                    const auto target_reg_size = get_bit_version(target_reg, target_size);

                    out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, from_size))
                       .make(m_add, reg_op(VSP), imm_op(bit_diff / 8))
                       .make(m_mov, mem_op(VSP, 0, target_size), reg_op(target_reg_size));
                }, cmd->get_current(), cmd->get_target());
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cnt_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        reg_size size = to_reg_size(ir_size);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const auto pop_reg = alloc_reg();
            auto pop_reg_size = get_bit_version(pop_reg, size);

            const bool is_bit8 = size == bit_8;
            if (is_bit8)
            {
                out
                    .make(m_xor, reg_op(pop_reg), reg_op(pop_reg))
                    .make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));

                pop_reg_size = get_bit_version(pop_reg, bit_16);
            }
            else out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));

            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));

            out.make(m_popcnt, reg_op(pop_reg_size), reg_op(pop_reg_size));

            if (is_bit8)
                pop_reg_size = get_bit_version(pop_reg, bit_8);

            out.make(m_mov, mem_op(VSP, 0, size), reg_op(pop_reg_size));
        }, cmd->get_size(), cmd->get_preserved());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_smul_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const reg pop_reg_one = alloc_reg();
            const reg pop_reg_one_size = get_bit_version(pop_reg_one, size);

            const reg pop_reg_two = alloc_reg();
            const reg pop_reg_two_size = get_bit_version(pop_reg_two, size);

            out.make(m_mov, reg_op(pop_reg_one_size), mem_op(VSP, 0, size));
            out.make(m_mov, reg_op(pop_reg_two_size), mem_op(VSP, TOB(size), size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));
            else
                out.make(m_add, reg_op(VSP), imm_op(size));

            out.make(m_imul, reg_op(pop_reg_two_size), reg_op(pop_reg_one_size))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(pop_reg_two_size));
        }, cmd->get_size(), cmd->get_preserved());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_umul_ptr& cmd)
    {
        VM_ASSERT("i am not doing this");
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_abs_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const reg pop_reg = alloc_reg();
            const reg pop_reg_size = get_bit_version(pop_reg, size);

            const reg shift_reg = alloc_reg();
            const reg shift_reg_size = get_bit_version(shift_reg, size);

            out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));

            const auto shift_size = static_cast<uint32_t>(size) - 1;
            out.make(m_mov, reg_op(shift_reg_size), reg_op(pop_reg_size))
               .make(m_sar, reg_op(shift_reg_size), imm_op(shift_size))
               .make(m_xor, reg_op(pop_reg_size), reg_op(shift_reg_size))
               .make(m_sub, reg_op(pop_reg_size), reg_op(shift_reg_size))

               .make(m_mov, mem_op(VSP, 0, size), reg_op(pop_reg_size));
        }, cmd->get_size(), cmd->get_preserved());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_log2_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const reg pop_reg = alloc_reg();
            reg pop_reg_size = get_bit_version(pop_reg, size);

            const reg count_reg = alloc_reg();
            reg count_reg_size = get_bit_version(count_reg, size);

            const bool is_bit8 = size == bit_8;
            if (is_bit8)
            {
                out
                    .make(m_xor, reg_op(pop_reg), reg_op(pop_reg))
                    .make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));

                count_reg_size = get_bit_version(count_reg, bit_16);
                pop_reg_size = get_bit_version(count_reg, bit_16);
            }
            else out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));

            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));

            out.make(m_xor, reg_op(count_reg_size), reg_op(count_reg_size))
               .make(m_bsr, reg_op(count_reg_size), reg_op(pop_reg_size))
               .make(m_cmovz, reg_op(count_reg_size), reg_op(pop_reg_size))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(count_reg_size));

            if (is_bit8)
                count_reg_size = get_bit_version(count_reg, bit_8);

            out.make(m_mov, mem_op(VSP, 0, size), reg_op(count_reg_size));
        }, cmd->get_size(), cmd->get_preserved());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_dup_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, block, cmd, [&](const asmb::code_container_ptr& out_container, const reg_allocator& alloc_reg)
        {
            encode_builder& out = *out_container;

            const reg target = get_bit_version(alloc_reg(), size);
            out.make(m_mov, reg_op(target), mem_op(VSP, 0, size))
               .make(m_sub, reg_op(VSP), imm_op(size))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(target));
        }, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_call_ptr& cmd)
    {
        auto& out = *block;

        VM_ASSERT(block_context.contains(cmd->get_target()), "target must be defined");
        const auto target_label = block_context[cmd->get_target()];
        const auto return_label = asmb::code_label::create();

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        out.make(m_lea, reg_op(VCS), mem_op(VCS, -8, bit_64))
           .make(m_mov, mem_op(VCS, 0, bit_64), imm_label_operand(return_label));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        out.make(m_mov, reg_op(VIP), imm_label_operand(target_label))
           .make(m_lea, reg_op(VIP), mem_op(VBASE, VIP, 1, 0, bit_64))
           .make(m_jmp, reg_op(VIP));

        // execution after VM handler should end up here
        out.label(return_label);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_ret_ptr& cmd)
    {
        auto& out = *block;

        out.make(m_mov, reg_op(VCSRET), mem_op(VCS, 0, bit_64))
           .make(m_lea, reg_op(VCS), mem_op(VCS, 8, bit_64))
           .make(m_lea, reg_op(VIP), mem_op(VBASE, VCSRET, 1, 0, bit_64))
           .make(m_jmp, reg_op(VIP));
    }

    std::vector<asmb::code_container_ptr> machine::create_handlers()
    {
        std::vector<asmb::code_container_ptr> out;
        for (auto& [_, pairs] : handler_map)
            for (auto& code : pairs | std::views::values)
                out.push_back(code);

        for (auto& [lable, dat] : misc_handlers)
            out.push_back(dat);

        return out;
    }

    reg machine::reg_vm_to_register(const ir::reg_vm store) const
    {
        ir::ir_size size = ir::ir_size::none;
        switch (store)
        {
            case ir::reg_vm::vip:
                size = ir::ir_size::bit_64;
                break;
            case ir::reg_vm::vip_32:
                size = ir::ir_size::bit_32;
                break;
            case ir::reg_vm::vip_16:
                size = ir::ir_size::bit_16;
                break;
            case ir::reg_vm::vip_8:
                size = ir::ir_size::bit_8;
                break;
            case ir::reg_vm::vsp:
                size = ir::ir_size::bit_64;
                break;
            case ir::reg_vm::vsp_32:
                size = ir::ir_size::bit_32;
                break;
            case ir::reg_vm::vsp_16:
                size = ir::ir_size::bit_16;
                break;
            case ir::reg_vm::vsp_8:
                size = ir::ir_size::bit_8;
                break;
            case ir::reg_vm::vbase:
                size = ir::ir_size::bit_64;
                break;
            default:
                VM_ASSERT("invalid case reached for reg_vm");
                break;
        }

        reg reg = none;
        switch (store)
        {
            case ir::reg_vm::vip:
            case ir::reg_vm::vip_32:
            case ir::reg_vm::vip_16:
            case ir::reg_vm::vip_8:
                reg = VIP;
                break;
            case ir::reg_vm::vsp:
            case ir::reg_vm::vsp_32:
            case ir::reg_vm::vsp_16:
            case ir::reg_vm::vsp_8:
                reg = VSP;
                break;
            case ir::reg_vm::vbase:
                reg = VBASE;
                break;
            default:
                VM_ASSERT("invalid case reached for reg_vm");
                break;
        }

        return get_bit_version(reg, to_reg_size(size));
    }

    void machine::handle_generic_logic_cmd(const mnemonic command, const ir::ir_size ir_size, const bool preserved,
        encode_builder& out, const reg_allocator& alloc_reg)
    {
        const reg_size size = to_reg_size(ir_size);
        const reg r_arg1 = get_bit_version(alloc_reg(), size);
        const reg r_arg_0 = get_bit_version(alloc_reg(), size);

        if (preserved)
        {
            out.make(m_mov, reg_op(r_arg1), mem_op(VSP, 0, size))
               .make(m_mov, reg_op(r_arg_0), mem_op(VSP, TOB(size), size));
        }
        else
        {
            out.make(m_mov, reg_op(r_arg1), mem_op(VSP, 0, size))
               .make(m_add, reg_op(VSP), imm_op(size))
               .make(m_mov, reg_op(r_arg_0), mem_op(VSP, 0, size))
               .make(m_add, reg_op(VSP), imm_op(size));
        }

        out.make(command, reg_op(r_arg_0), reg_op(r_arg1))
           .make(m_sub, reg_op(VSP), imm_op(size))
           .make(m_mov, mem_op(VSP, 0, size), reg_op(r_arg_0));
    }

    void machine::create_handler(const handler_call_flags flags, const asmb::code_container_ptr& block, const handler_generator& create,
        const size_t handler_hash)
    {
        scope_register_manager scope = reg_64_container->create_scope();
        auto reg_allocator = [&]() -> reg
        {
            return scope.reserve();
        };

        // if not force inlined, we let the vm settings decide the chance of generating a new handler
        if (flags != force_inline)
        {
            asmb::code_label_ptr target_label = nullptr;
            if (flags != default_create)
            {
            HANDLE_CREATE:
                asmb::code_container_ptr builder = asmb::code_container::create();
                target_label = asmb::code_label::create(std::to_string(handler_hash));

                builder->bind_start(target_label);
                create(builder, reg_allocator);
                builder->make(m_mov, reg_op(VCSRET), mem_op(VCS, 0, bit_64))
                       .make(m_lea, reg_op(VCS), mem_op(VCS, 8, bit_64))
                       .make(m_lea, reg_op(VIP), mem_op(VBASE, VCSRET, 1, 0, bit_64))
                       .make(m_jmp, reg_op(VIP));

                handler_map[handler_hash].emplace_back(target_label, builder);
            }
            else
            {
                constexpr float chance_to_generate = 0.3;
                if (!(flags & force_unique) && (handler_map[handler_hash].empty() || util::get_ran_device().gen_chance(chance_to_generate)))
                    goto HANDLE_CREATE;

                const auto& handler_instances = handler_map[handler_hash];
                target_label = std::get<0>(util::get_ran_device().random_elem(handler_instances));
            }

            // write the call into the current block
            const asmb::code_label_ptr return_label = asmb::code_label::create();
            encode_builder& out = *block;

            // lea VCS, [VCS - 8]       ; allocate space for new return address
            // mov [VCS], code_label    ; place return rva on the stack
            out.make(m_lea, reg_op(VCS), mem_op(VCS, -8, bit_64))
               .make(m_mov, mem_op(VCS, 0, bit_64), imm_label_operand(return_label));

            // lea VIP, [VBASE + VCSRET]  ; add rva to base
            // jmp VIP
            out.make(m_mov, reg_op(VIP), imm_label_operand(target_label))
               .make(m_lea, reg_op(VIP), mem_op(VBASE, VIP, 1, 0, bit_64))
               .make(m_jmp, reg_op(VIP));

            // execution after VM handler should end up here
            out.label(return_label);
        }
        else
        {
            // inline into current block
            create(block, reg_allocator);
        }
    }
}
