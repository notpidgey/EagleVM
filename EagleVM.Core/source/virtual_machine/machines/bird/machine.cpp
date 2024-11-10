#include "eaglevm-core/virtual_machine/machines/bird/machine.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

#include <unordered_set>

#include "eaglevm-core/virtual_machine/machines/util.h"
#include "eaglevm-core/virtual_machine/machines/bird/handler.h"
#include "eaglevm-core/virtual_machine/machines/bird/loader.h"

#define VIP reg_man->get_vm_reg(register_manager::index_vip)
#define VSP reg_man->get_vm_reg(register_manager::index_vsp)
#define VREGS reg_man->get_vm_reg(register_manager::index_vregs)
#define VCS reg_man->get_vm_reg(register_manager::index_vcs)
#define VCSRET reg_man->get_vm_reg(register_manager::index_vcsret)
#define VBASE reg_man->get_vm_reg(register_manager::index_vbase)
#define VFLAGS reg_man->get_vm_reg(register_manager::index_vflags)

#define VTEMP reg_man->get_reserved_temp(0)
#define VTEMP2 reg_man->get_reserved_temp(1)
#define VTEMPX(x) reg_man->get_reserved_temp(x)

namespace eagle::virt::eg
{
    using namespace codec;
    using namespace codec::encoder;

    bird_machine::bird_machine(const settings_ptr& settings_info)
    {
        settings = settings_info;
        out_block = nullptr;
    }

    bird_machine_ptr bird_machine::create(const settings_ptr& settings_info)
    {
        const std::shared_ptr<bird_machine> instance = std::make_shared<bird_machine>(settings_info);
        const std::shared_ptr<register_manager> reg_man = std::make_shared<register_manager>(settings_info);
        reg_man->init_reg_order();
        reg_man->create_mappings();

        const std::shared_ptr<register_context> reg_ctx_64 = std::make_shared<register_context>(reg_man->get_unreserved_temp(), codec::gpr_64);
        const std::shared_ptr<register_context> reg_ctx_128 = std::make_shared<register_context>(reg_man->get_unreserved_temp_xmm(), codec::xmm_128);
        // const std::shared_ptr<handler_manager> han_man = std::make_shared<handler_manager>(instance, reg_man, reg_ctx_64, reg_ctx_128, settings_info);

        instance->reg_man = reg_man;
        instance->reg_64_container = reg_ctx_64;
        instance->reg_128_container = reg_ctx_128;
        // instance->han_man = han_man;

        return instance;
    }

    asmb::code_container_ptr bird_machine::lift_block(const ir::block_ptr& block)
    {
        const size_t command_count = block->size();

        const asmb::code_container_ptr code = asmb::code_container::create("block_begin " + std::to_string(command_count), true);
        if (block_context.contains(block))
        {
            const asmb::code_label_ptr label = block_context[block];
            code->bind(label);
        }

        // walk backwards each command to see the last time each discrete_ptr was used
        // this will tell us when we can free the register
        std::vector<std::vector<ir::discrete_store_ptr>> store_dead(command_count, { });
        std::unordered_set<ir::discrete_store_ptr> discovered_stores;

        // as a fair warning, this analysis only cares about the store usage in the current block
        // so if you are using a store across multiple blocks, you can wish that goodbye because i will not
        // be implementing that because i do not need it
        for (auto i = command_count; i--;)
        {
            auto ref = block->at(i)->get_use_stores();
            for (auto& store : ref)
            {
                if (!discovered_stores.contains(store))
                {
                    store_dead[i].push_back(store);
                    discovered_stores.insert(store);
                }
            }
        }

        for (size_t i = 0; i < command_count; i++)
        {
            const ir::base_command_ptr command = block->at(i);
            dispatch_handle_cmd(code, command);

            for (auto store : store_dead[i])
                reg_64_container->release(store);
        }

        reg_64_container->reset();
        reg_128_container->reset();

        return code;
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_load_ptr& cmd)
    {
        const auto load_reg = cmd->get_reg();
        if (get_reg_class(load_reg) == seg)
        {
            create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
                out.make(m_sub, reg_op(VSP), imm_op(bit_64))
                   .make(m_mov, mem_op(VSP, 0, bit_64), reg_op(load_reg));
            });
        }
        else
        {
            create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
                const auto size = get_reg_size(load_reg);
                const auto output_reg = get_bit_version(alloc_reg(), size);

                const register_loader loader(regs);
                loader.load_register(load_reg, output_reg, out);

                out
                    .make(m_sub, reg_op(VSP), imm_op(size))
                    .make(m_mov, mem_op(VSP, 0, size), reg_op(output_reg));
            });
        }
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_store_ptr& cmd)
    {
        const auto store_reg = cmd->get_reg();
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const auto size = get_reg_size(store_reg);
            const auto output_reg = get_bit_version(alloc_reg(), size);

            out
                .make(m_mov, reg_op(output_reg), mem_op(VSP, 0, size))
                .make(m_add, reg_op(VSP), imm_op(size));

            const register_loader loader(regs);
            loader.store_register(store_reg, output_reg, out);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_branch_ptr& cmd)
    {
        ir::exit_condition condition = cmd->get_condition();
        std::vector<ir::ir_exit_result> push_order =
            condition != ir::exit_condition::jmp
                ? std::vector{ cmd->get_condition_special(), cmd->get_condition_default() }
                : std::vector{ cmd->get_condition_default() };

        // if the condition is inverted, we want the opposite branches to be taken
        if (condition != ir::exit_condition::jmp && cmd->is_inverted())
            std::swap(push_order[0], push_order[1]);

        if (cmd->is_virtual())
        {
            create_handler(force_inline, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
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
            create_handler(force_inline, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
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

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_handler_call_ptr& cmd)
    {
        // todo: pull the actual handler
        asmb::code_label_ptr handler_label = nullptr;
        if (cmd->is_operand_sig())
        {
            const auto sig = cmd->get_x86_signature();
            // find_handler(sig)
        }
        else
        {
            const auto sig = cmd->get_handler_signature();
            // find_handler(sig)
        }

        create_handler(force_inline, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const asmb::code_label_ptr return_label = asmb::code_label::create();

            out.make(m_lea, reg_op(VCS), mem_op(VCS, -8, bit_64))
               .make(m_mov, mem_op(VCS, 0, bit_64), imm_label_operand(return_label))

               .make(m_mov, reg_op(VIP), imm_label_operand(handler_label))
               .make(m_lea, reg_op(VIP), mem_op(VBASE, VIP, 1, 0, bit_64))
               .make(m_jmp, reg_op(VIP))

               .label(return_label);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_read_ptr& cmd)
    {
        const ir::ir_size value_size = cmd->get_read_size();
        const auto value_reg_size = to_reg_size(value_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const auto address_reg = alloc_reg();
            const auto value_reg = get_bit_version(alloc_reg(), value_reg_size);

            out.make(m_mov, reg_op(address_reg), mem_op(VSP, 0, bit_64))
               .make(m_add, reg_op(VSP), imm_op(value_reg_size))
               .make(m_mov, reg_op(value_reg), mem_op(address_reg, 0, value_reg_size))

               .make(m_sub, reg_op(VSP), imm_op(value_reg_size))
               .make(m_mov, mem_op(VSP, 0, value_reg), reg_op(value_reg));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_write_ptr& cmd)
    {
        // TODO: there should not be a difference between value and write size... remove this
        const ir::ir_size value_size = cmd->get_value_size();
        const auto value_reg_size = to_reg_size(value_size);

        if (cmd->get_is_value_nearest())
        {
            create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
                const auto mem_reg = alloc_reg();
                const auto value_reg = get_bit_version(alloc_reg(), value_reg_size);

                out.make(m_mov, reg_op(value_reg), mem_op(VSP, 0, value_reg_size))
                   .make(m_add, reg_op(VSP), imm_op(value_reg_size))

                   .make(m_mov, reg_op(mem_reg), mem_op(VSP, 0, 8))
                   .make(m_add, reg_op(VSP), imm_op(bit_64))

                   .make(m_mov, mem_op(mem_reg, 0, value_reg_size), reg_op(value_reg));
            });
        }
        else
        {
            create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
                const auto mem_reg = alloc_reg();
                const auto value_reg = get_bit_version(alloc_reg(), value_reg_size);

                out.make(m_mov, reg_op(mem_reg), mem_op(VSP, 0, 8))
                   .make(m_add, reg_op(VSP), imm_op(bit_64))

                   .make(m_mov, reg_op(value_reg), mem_op(VSP, 0, value_reg_size))
                   .make(m_add, reg_op(VSP), imm_op(value_reg_size))

                   .make(m_mov, mem_op(mem_reg, 0, value_reg_size), reg_op(value_reg));
            });
        }
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_pop_ptr& cmd)
    {
        VM_ASSERT(cmd->get_destination_reg() == nullptr, "unsupported instruction type");

        const auto pop_size = cmd->get_size();
        const auto pop_reg_size = to_reg_size(pop_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            out.make(m_add, reg_op(VSP), imm_op(pop_reg_size));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_push_ptr& cmd)
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

                create_handler(force_inline, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
                {
                    const auto reg = get_bit_version(alloc_reg(), push_reg_size);
                    out.make(m_mov, reg_op(reg), imm_op(immediate_value))
                       .make(m_sub, reg_op(VSP), imm_op(push_reg_size))
                       .make(m_mov, mem_op(VSP, 0, push_reg_size), reg_op(reg));
                });
            }
            else if constexpr (std::is_same_v<T, ir::block_ptr>)
            {
                const ir::block_ptr& target = arg;
                const asmb::code_label_ptr label = get_block_label(target);
                VM_ASSERT(label != nullptr, "block contains missing context");

                create_handler(force_inline, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
                {
                    const auto reg = get_bit_version(alloc_reg(), push_reg_size);
                    out.make(m_mov, reg_op(reg), imm_label_operand(label))
                       .make(m_sub, reg_op(VSP), imm_op(push_reg_size))
                       .make(m_mov, mem_op(VSP, 0, push_reg_size), reg_op(reg));
                });
            }
            else if constexpr (std::is_same_v<T, ir::reg_vm>)
            {
                const ir::reg_vm& target = arg;
                const auto vm_reg = reg_vm_to_register(target);

                create_handler(force_inline, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
                {
                    const auto reg = get_bit_version(alloc_reg(), push_reg_size);
                    out.make(m_mov, reg_op(reg), reg_op(vm_reg))
                       .make(m_sub, reg_op(VSP), imm_op(push_reg_size))
                       .make(m_mov, mem_op(VSP, 0, push_reg_size), reg_op(reg));
                });
            }
            else
            VM_ASSERT("deprecated command type");
        }, push_value);
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_load_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            // prepare to pop flags from stack
            out.make(m_sub, reg_op(rsp), imm_op(bit_64))
               .make(m_popfq)

               // set rsp to be vsp
               .make(m_xchg, reg_op(VSP), reg_op(rsp))

               // push flags to virtual stack
               .make(m_pushfq)

               // reset rsp
               .make(m_xchg, reg_op(VSP), reg_op(rsp));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_store_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const auto mask_reg = alloc_reg();
            const auto flag_reg = alloc_reg();

            out.make(m_mov, reg_op(mask_reg), mem_op(VSP, 0, bit_64))
               .make(m_mov, reg_op(mask_reg), mem_op(VSP, 8, bit_64))

               // mask off the wanted bits from flags
               .make(m_and, reg_op(flag_reg), reg_op(mask_reg))

               // flip the mask, and remove unwanted bits from destination
               .make(m_not, reg_op(mask_reg))
               .make(m_and, mem_op(rsp, -8, bit_64), reg_op(mask_reg))

               // combine
               .make(m_or, mem_op(rsp, -8, bit_64), reg_op(flag_reg))

               // pop the mask
               .make(m_add, reg_op(VSP), imm_op(bit_64));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sx_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const auto temp_reg = alloc_reg();
            const auto from_size = to_reg_size(cmd->get_current());
            const auto to_size = to_reg_size(cmd->get_target());

            const reg from_reg = get_bit_version(temp_reg, from_size);
            const reg to_reg = get_bit_version(temp_reg, to_size);

            out.make(m_mov, reg_op(from_reg), mem_op(VSP, 0, from_size))
               .make(m_movsx, reg_op(to_reg), reg_op(from_reg))
               .make(m_mov, mem_op(VSP, 0, to_size), reg_op(to_reg));

            const auto stack_diff = static_cast<uint32_t>(to_size) - static_cast<uint32_t>(from_size);
            const auto byte_diff = stack_diff / 8;

            out.make(m_sub, reg_op(VSP), imm_op(byte_diff))
               .make(m_mov, mem_op(VSP, 0, to_size), reg_op(to_reg));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_dynamic_ptr& cmd)
    {
        VM_ASSERT("deprecated command type");
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd)
    {
        VM_ASSERT("deprecated command type");
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_flags_load_ptr& cmd)
    {
        const reg vflag_reg = reg_man->get_vm_reg(register_manager::index_vflags);
        const uint32_t flag_index = ir::cmd_flags_load::get_flag_index(cmd->get_flag());

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const reg flag_hold = alloc_reg();
            out.make(m_mov, reg_op(flag_hold), reg_op(vflag_reg))
               .make(m_shr, reg_op(flag_hold), imm_op(flag_index))
               .make(m_and, reg_op(flag_hold), imm_op(1))

               .make(m_sub, reg_op(VSP), imm_op(bit_64))
               .make(m_mov, mem_op(VSP, 0, bit_64), reg_op(flag_hold));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_jmp_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const reg pop_reg = alloc_reg();
            out.make(m_mov, reg_op(pop_reg), mem_op(VSP, 0, bit_64))
               .make(m_jmp, ZREG(pop_reg));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_and_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            handle_generic_logic_cmd(m_and, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_or_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            handle_generic_logic_cmd(m_or, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_xor_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            handle_generic_logic_cmd(m_xor, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shl_ptr& cmd)
    {
        const reg_size size = to_reg_size(cmd->get_size());
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const auto value_reg = get_bit_version(alloc_reg(), size);
            const auto shift_reg = get_bit_version(alloc_reg(), size);

            out.make(m_mov, reg_op(shift_reg), mem_op(VSP, 0, size));
            out.make(m_mov, reg_op(shift_reg), mem_op(VSP, TOB(size), size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(shift_reg), imm_op(size));
            else
                out.make(m_add, reg_op(shift_reg), imm_op(size));

            out.make(m_shlx, reg_op(value_reg), reg_op(value_reg), reg_op(shift_reg))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(value_reg));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shr_ptr& cmd)
    {
        const reg_size size = to_reg_size(cmd->get_size());
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const auto value_reg = get_bit_version(alloc_reg(), size);
            const auto shift_reg = get_bit_version(alloc_reg(), size);

            out.make(m_mov, reg_op(shift_reg), mem_op(VSP, 0, size));
            out.make(m_mov, reg_op(shift_reg), mem_op(VSP, TOB(size), size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(shift_reg), imm_op(size));
            else
                out.make(m_add, reg_op(shift_reg), imm_op(size));

            out.make(m_shrx, reg_op(value_reg), reg_op(value_reg), reg_op(shift_reg))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(value_reg));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_add_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            handle_generic_logic_cmd(m_add, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sub_ptr& cmd)
    {
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            handle_generic_logic_cmd(m_sub, cmd->get_size(), cmd->get_preserved(), out, alloc_reg);
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cmp_ptr& cmd)
    {
        const reg_size size = to_reg_size(cmd->get_size());
        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
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
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_resize_ptr& cmd)
    {
        const reg_size from_size = to_reg_size(cmd->get_current());
        const reg_size target_size = to_reg_size(cmd->get_target());

        if (target_size > from_size)
        {
            const auto bit_diff = static_cast<uint32_t>(target_size) - static_cast<uint32_t>(from_size);
            create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
                const auto target_reg = alloc_reg();
                const auto pop_reg_size = get_bit_version(target_reg, from_size);
                const auto target_reg_size = get_bit_version(target_reg, target_size);

                out.make(m_xor, reg_op(target_reg_size), reg_op(target_reg_size))
                   .make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, from_size))
                   .make(m_sub, reg_op(VSP), imm_op(bit_diff / 8))
                   .make(m_mov, mem_op(VSP, 0, target_size), imm_op(target_reg_size));
            });
        }
        else if (from_size > target_size)
        {
            const auto bit_diff = static_cast<uint32_t>(from_size) - static_cast<uint32_t>(target_size);
            create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
            {
                const auto target_reg = alloc_reg();
                const auto pop_reg_size = get_bit_version(target_reg, from_size);
                const auto target_reg_size = get_bit_version(target_reg, target_size);

                out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, from_size))
                   .make(m_add, reg_op(VSP), imm_op(bit_diff / 8))
                   .make(m_mov, mem_op(VSP, 0, target_size), imm_op(target_reg_size));
            });
        }
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cnt_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const reg pop_reg = alloc_reg();
            reg pop_reg_size = get_bit_version(pop_reg, size);

            const bool is_bit8 = size == bit_8;
            if (is_bit8)
            {
                pop_reg_size = get_bit_version(pop_reg, bit_16);
                out.make(m_xor, reg_op(pop_reg_size), reg_op(pop_reg_size));

                pop_reg_size = get_bit_version(pop_reg, bit_8);
            }

            out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));
            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));

            if (is_bit8)
            {
                pop_reg_size = get_bit_version(pop_reg, bit_16);
                out.make(m_popcnt, reg_op(pop_reg_size), reg_op(pop_reg_size));

                pop_reg_size = get_bit_version(pop_reg, bit_8);
            }
            else out.make(m_popcnt, reg_op(pop_reg_size), reg_op(pop_reg_size));

            out.make(m_mov, mem_op(VSP, 0, size), reg_op(pop_reg_size));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_smul_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
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
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_umul_ptr& cmd)
    {
        VM_ASSERT("i am not doing this");
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_abs_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
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
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_log2_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const reg pop_reg = alloc_reg();
            reg pop_reg_size = get_bit_version(pop_reg, size);

            const reg count_reg = alloc_reg();
            reg count_reg_size = get_bit_version(count_reg, size);

            if (size == bit_8)
            {
                const auto scaled_reg = get_bit_version(pop_reg, bit_16);
                out.make(m_xor, reg_op(scaled_reg), reg_op(scaled_reg))
                   .make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));

                pop_reg_size = get_bit_version(pop_reg, bit_16);
                count_reg_size = get_bit_version(count_reg, bit_16);
            }
            else out.make(m_mov, reg_op(pop_reg_size), mem_op(VSP, 0, size));

            if (cmd->get_preserved())
                out.make(m_sub, reg_op(VSP), imm_op(size));

            out.make(m_bsr, reg_op(count_reg_size), reg_op(pop_reg_size))
               .make(m_cmovz, reg_op(count_reg_size), reg_op(pop_reg_size));

            if (size == bit_8)
            {
                count_reg_size = get_bit_version(count_reg, bit_8);
                out.make(m_mov, mem_op(VSP, 0, bit_8), reg_op(count_reg_size));
            }
            else out.make(m_mov, mem_op(VSP, 0, size), reg_op(count_reg_size));
        });
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_dup_ptr& cmd)
    {
        const ir::ir_size ir_size = cmd->get_size();
        const reg_size size = to_reg_size(ir_size);

        create_handler(default_create, cmd->get_command_type(), [&](encode_builder& out, const std::function<reg()>& alloc_reg)
        {
            const reg target = get_bit_version(alloc_reg(), size);
            out.make(m_mov, reg_op(target), mem_op(VSP, 0, size))
               .make(m_sub, reg_op(VSP), imm_op(size))
               .make(m_mov, mem_op(VSP, 0, size), reg_op(target));
        });
    }

    reg bird_machine::reg_vm_to_register(const ir::reg_vm store) const
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

    void bird_machine::handle_generic_logic_cmd(const mnemonic command, const ir::ir_size ir_size, const bool preserved,
        encode_builder& out, const std::function<reg()>& alloc_reg)
    {
        const reg_size size = to_reg_size(ir_size);
        const reg r_par1 = get_bit_version(alloc_reg(), size);
        const reg r_par2 = get_bit_version(alloc_reg(), size);

        if (preserved)
        {
            out.make(m_mov, reg_op(r_par1), mem_op(VSP, 0, size))
               .make(m_mov, reg_op(r_par2), mem_op(VSP, TOB(size), size));
        }
        else
        {
            out.make(m_mov, reg_op(r_par1), mem_op(VSP, 0, size))
               .make(m_add, reg_op(r_par1), imm_op(size))
               .make(m_mov, reg_op(r_par2), mem_op(VSP, 0, size))
               .make(m_add, reg_op(r_par2), imm_op(size));
        }

        out.make(command, reg_op(r_par1), reg_op(r_par2))
           .make(m_sub, reg_op(VSP), imm_op(size))
           .make(m_mov, mem_op(VSP, 0, size), reg_op(r_par1));
    }

    void bird_machine::create_handler(handler_call_flags flags, ir::command_type cmd_type,
        std::function<void(codec::encoder::encode_builder&, std::function<codec::reg()>)> handler_create)
    {
        scope_register_manager scope = reg_64_container->create_scope();

        auto reg_allocator = [&]
        {
            return scope.reserve();
        };

        // if not force inlined, we let the vm settings decide the chance of generating a new handler
        if (flags)
        {
            // dump to block
            auto builder = std::make_shared<encode_builder>();
            handler_create(builder, reg_allocator);

            // todo: actually add to block
        }
        else
        {
            constexpr float chance_to_generate = 0.10;
            if (util::get_ran_device().gen_chance(chance_to_generate))
            {
                // we want to generate a new handler
                encode_builder_ptr builder = std::make_shared<encode_builder>();
                handler_create(builder, reg_allocator);

                // create a handler return
                builder->make(m_mov, reg_op(VCSRET), mem_op(VCS, 0, bit_64))
                       .make(m_lea, reg_op(VCS), mem_op(VCS, 8, bit_64))
                       .make(m_lea, reg_op(VIP), mem_op(VBASE, VCSRET, 1, 0, bit_64))
                       .make(m_jmp, reg_op(VIP));
            }
            else
            {
                // we want to use an existing handler
            }
        }
    }
}
