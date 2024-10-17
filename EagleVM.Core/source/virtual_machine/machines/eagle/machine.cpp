#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/machines/eagle/handler_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

#include "eaglevm-core/virtual_machine/machines/register_context.h"

#include "eaglevm-core/virtual_machine/machines/util.h"

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

using namespace eagle::codec;

namespace eagle::virt::eg
{
    machine::machine(const settings_ptr& settings_info) { settings = settings_info; }

    machine_ptr machine::create(const settings_ptr& settings_info)
    {
        const std::shared_ptr<machine> instance = std::make_shared<machine>(settings_info);
        const std::shared_ptr<register_manager> reg_man = std::make_shared<register_manager>(settings_info);
        reg_man->init_reg_order();
        reg_man->create_mappings();

        const std::shared_ptr<register_context> reg_ctx_64 = std::make_shared<register_context>(reg_man->get_unreserved_temp(), gpr_64);
        const std::shared_ptr<register_context> reg_ctx_128 = std::make_shared<register_context>(reg_man->get_unreserved_temp_xmm(), xmm_128);
        const std::shared_ptr<handler_manager> han_man = std::make_shared<handler_manager>(instance, reg_man, reg_ctx_64, reg_ctx_128, settings_info);

        instance->reg_man = reg_man;
        instance->reg_64_container = reg_ctx_64;
        instance->reg_128_container = reg_ctx_128;
        instance->han_man = han_man;

        return instance;
    }

    asmb::code_container_ptr machine::lift_block(const ir::block_ptr& block)
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
            handle_cmd(code, command);

            for (auto store : store_dead[i])
                reg_64_container->release(store);
        }

        reg_64_container->reset();
        reg_128_container->reset();

        return code;
    }

    void machine::handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command) { base_machine::handle_cmd(code, command); }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_load_ptr& cmd)
    {
        // we want to load this register onto the stack
        const reg load_reg = cmd->get_reg();

        ir::discrete_store_ptr dest = nullptr;
        if (get_reg_class(load_reg) == seg)
        {
            dest = ir::discrete_store::create(ir::ir_size::bit_64);
            reg_64_container->assign(dest);

            block->add(encode(m_mov, ZREG(dest->get_store_register()), ZREG(load_reg)));
        }
        else
        {
            const reg_size load_reg_size = get_reg_size(load_reg);

            dest = ir::discrete_store::create(to_ir_size(load_reg_size));
            reg_64_container->assign(dest);

            // load into storage
            if (settings->complex_temp_loading)
            {
                auto [handler_load, working_reg_res, complex_mapping] = han_man->load_register_complex(load_reg, dest);

                // load storage <- target_reg
                han_man->call_vm_handler(block, handler_load);

                // resolve target_reg
                const auto handler_resolve = han_man->resolve_complexity(dest, complex_mapping);
                han_man->call_vm_handler(block, handler_resolve);
            }
            else
            {
                auto [handler_load, working_reg_res] = han_man->load_register(load_reg, dest);

                block->add(encode(m_xor, ZREG(dest->get_store_register()), ZREG(dest->get_store_register())));

                // load storage <- target_reg
                han_man->call_vm_handler(block, handler_load);
            }
        }

        // push target_reg
        call_push(block, dest);
        reg_64_container->release(dest);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_store_ptr& cmd)
    {
        // we want to store a value into this register from the stack
        reg target_reg = cmd->get_reg();
        auto r_size = get_reg_size(target_reg);
        auto v_size = cmd->get_value_size();

        const ir::discrete_store_ptr storage = ir::discrete_store::create(to_ir_size(r_size));
        reg_64_container->assign(storage);

        // pop into storage
        call_pop(block, storage, v_size);

        // store into target
        auto [handler, working_reg] = han_man->store_register(target_reg, storage);
        if (working_reg != storage->get_store_register())
            block->add(encode(m_mov, ZREG(storage->get_store_register()), ZREG(working_reg)));

        han_man->call_vm_handler(block, handler);
        reg_64_container->release(storage);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_branch_ptr& cmd)
    {
        // the inverted (second) mnemonic is useless for now, but im going to keep it here anyways
        std::unordered_map<ir::exit_condition, std::array<codec::mnemonic, 2>> jcc_lookup =
        {
            { ir::exit_condition::jo, { codec::mnemonic::m_jo, codec::mnemonic::m_jno } },
            { ir::exit_condition::js, { codec::mnemonic::m_js, codec::mnemonic::m_jns } },
            { ir::exit_condition::je, { codec::mnemonic::m_jz, codec::mnemonic::m_jnz } },
            { ir::exit_condition::jb, { codec::mnemonic::m_jb, codec::mnemonic::m_jnb } },
            { ir::exit_condition::jbe, { codec::mnemonic::m_jbe, codec::mnemonic::m_jnbe } },
            { ir::exit_condition::jl, { codec::mnemonic::m_jl, codec::mnemonic::m_jnl } },
            { ir::exit_condition::jle, { codec::mnemonic::m_jle, codec::mnemonic::m_jnle } },
            { ir::exit_condition::jp, { codec::mnemonic::m_jp, codec::mnemonic::m_jnp } },

            { ir::exit_condition::jcxz, { codec::mnemonic::m_jcxz, codec::mnemonic::m_invalid } },
            { ir::exit_condition::jecxz, { codec::mnemonic::m_jecxz, codec::mnemonic::m_invalid } },
            { ir::exit_condition::jrcxz, { codec::mnemonic::m_jrcxz, codec::mnemonic::m_invalid } },

            { ir::exit_condition::jmp, { codec::mnemonic::m_jmp, codec::mnemonic::m_invalid } }
        };

        ir::exit_condition cmd_condition = cmd->get_condition();
        std::vector<ir::ir_exit_result> push_order =
            cmd_condition != ir::exit_condition::jmp
                ? std::vector{ cmd->get_condition_special(), cmd->get_condition_default() }
                : std::vector{ cmd->get_condition_default() };

        // if the condition is inverted, we want the opposite branches to be taken
        if (cmd_condition != ir::exit_condition::jmp && cmd->is_inverted())
            std::swap(push_order[0], push_order[1]);

        // push all
        for (const ir::ir_exit_result& exit_result : push_order)
        {
            std::visit([&](auto&& arg)
            {
                scope_register_manager scope = reg_64_container->create_scope();
                const reg temp_reg = scope.reserve();

                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, uint64_t>)
                {
                    const uint64_t immediate_value = arg;

                    block->add(encode(m_mov, ZREG(temp_reg), ZIMMU(immediate_value)));
                    call_push(block, get_bit_version(temp_reg, bit_64));
                }
                else if constexpr (std::is_same_v<T, ir::block_ptr>)
                {
                    const ir::block_ptr target = arg;

                    const asmb::code_label_ptr label = get_block_label(target);
                    VM_ASSERT(label != nullptr, "block must not be pointing to null label, missing context");

                    block->add(RECOMPILE(encode(m_mov, ZREG(temp_reg), ZIMMU(label->get_address()))));
                    call_push(block, get_bit_version(temp_reg, bit_64));
                }
                else
                {
                    VM_ASSERT("unimplemented exit result");
                }
            }, exit_result);
        }

        // this is a hacky solution but the exit condition maps to an actual handler id.
        // so we can just cast the exit_condition to a uint64_t and that will give us the handler id :)
        // we also want to inline this so we are inserting it in place
        ir::ir_insts jcc_instructions = han_man->build_instruction_handler(codec::m_jmp, uint64_t(cmd_condition));
        for (auto& inst : jcc_instructions)
            handle_cmd(block, inst);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_handler_call_ptr& cmd)
    {
        if (cmd->is_operand_sig())
        {
            const auto sig = cmd->get_x86_signature();
            han_man->call_vm_handler(block, han_man->get_instruction_handler(cmd->get_mnemonic(), sig));
        }
        else
        {
            const auto sig = cmd->get_handler_signature();
            han_man->call_vm_handler(block, han_man->get_instruction_handler(cmd->get_mnemonic(), sig));
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_read_ptr& cmd)
    {
        ir::ir_size target_size = cmd->get_read_size();

        scope_register_manager scope = reg_64_container->create_scope();
        reg temp = scope.reserve();
        reg target_temp = get_bit_version(temp, static_cast<reg_size>(target_size));

        // pop address
        call_pop(block, temp);

        // mov temp, [address]
        block->add(encode(m_mov, ZREG(target_temp), ZMEMBD(target_temp, 0, TOB(target_size))));

        // push
        call_push(block, get_bit_version(temp, to_reg_size(target_size)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_write_ptr& cmd)
    {
        const ir::ir_size value_size = cmd->get_value_size();
        const ir::ir_size write_size = cmd->get_write_size();

        scope_register_manager scope = reg_64_container->create_scope();
        const auto temps = scope.reserve<2>();

        const reg temp_value = get_bit_version(temps[0], to_reg_size(value_size));
        const reg temp_address = temps[1];

        if (cmd->get_is_value_nearest())
        {
            call_pop(block, temp_value);
            call_pop(block, temp_address);
        }
        else
        {
            call_pop(block, temp_address);
            call_pop(block, temp_value);
        }

        block->add(encode(m_mov, ZMEMBD(temp_address, 0, TOB(write_size)), ZREG(temp_value)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_pop_ptr& cmd)
    {
        if (const ir::discrete_store_ptr store = cmd->get_destination_reg())
        {
            reg_64_container->assign(store);
            call_pop(block, store);
        }
        else
        {
            scope_register_manager scope = reg_64_container->create_scope();
            call_pop(block, scope.reserve());
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_push_ptr& cmd)
    {
        // call ia32 handler for push
        auto push_val = cmd->get_value();

        scope_register_manager scope = reg_64_container->create_scope();
        std::visit([&]<typename push_type>(push_type&& arg)
        {
            using T = std::decay_t<push_type>;
            if constexpr (std::is_same_v<T, uint64_t>)
            {
                const uint64_t immediate_value = arg;

                const reg temp_reg = scope.reserve();

                block->add(encode(m_mov, ZREG(temp_reg), ZIMMU(immediate_value)));
                call_push(block, get_bit_version(temp_reg, to_reg_size(cmd->get_size())));
            }
            else if constexpr (std::is_same_v<T, ir::block_ptr>)
            {
                const ir::block_ptr& target = arg;

                const asmb::code_label_ptr label = get_block_label(target);
                VM_ASSERT(label != nullptr, "block contains missing context");

                const reg temp_reg = scope.reserve();

                block->add(RECOMPILE(encode(m_mov, ZREG(temp_reg), ZIMMU(label->get_address()))));
                call_push(block, get_bit_version(temp_reg, to_reg_size(cmd->get_size())));
            }
            else if constexpr (std::is_same_v<T, ir::discrete_store_ptr>)
            {
                const ir::discrete_store_ptr& target = arg;
                VM_ASSERT(target->get_finalized(), "label must be finalized before pushing");

                call_push(block, get_bit_version(target->get_store_register(), to_reg_size(cmd->get_size())));
            }
            else if constexpr (std::is_same_v<T, ir::reg_vm>)
            {
                const ir::reg_vm& target = arg;

                const auto vm_reg = reg_vm_to_register(target);
                call_push(block, vm_reg);
            }
            else
            {
                VM_ASSERT("reached invalid stack_type for push command");
            }
        }, push_val);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_load_ptr&)
    {
        const asmb::code_label_ptr vm_rflags_load = han_man->get_rlfags_load();
        han_man->call_vm_handler(block, vm_rflags_load);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_store_ptr& store)
    {
        const asmb::code_label_ptr vm_rflags_store = han_man->get_rflags_store();

        auto scope = reg_64_container->create_scope();
        auto reserved = scope.reserve();

        block->add(encode(m_mov, ZREG(reserved), ZIMMS(store->get_relevant_flags())));
        call_push(block, reserved);

        han_man->call_vm_handler(block, vm_rflags_store);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sx_ptr& cmd)
    {
        const ir::ir_size ir_current_size = cmd->get_current();
        reg_size current_size = to_reg_size(ir_current_size);

        const ir::ir_size ir_target_size = cmd->get_target();
        const reg_size target_size = to_reg_size(ir_target_size);

        scope_register_manager scope = reg_64_container->create_scope();
        const reg temp_reg = scope.reserve();
        call_pop(block, get_bit_version(temp_reg, current_size));

        // mov eax/ax/al, VTEMP
        block->add(encode(m_mov, ZREG(get_bit_version(rax, current_size)), ZREG(get_bit_version(temp_reg, current_size))));

        // keep upgrading the operand until we get to destination size
        while (current_size != target_size)
        {
            // other sizes should not be possible
            switch (current_size)
            {
                case bit_32:
                {
                    block->add(encode(m_cdqe));
                    current_size = bit_64;
                    break;
                }
                case bit_16:
                {
                    block->add(encode(m_cwde));
                    current_size = bit_32;
                    break;
                }
                case bit_8:
                {
                    block->add(encode(m_cbw));
                    current_size = bit_16;
                    break;
                }
                default:
                {
                    VM_ASSERT("unable to handle size translation");
                }
            }
        }


        block->add(encode(m_mov, ZREG(get_bit_version(temp_reg, current_size)), ZREG(get_bit_version(rax, current_size))));
        call_push(block, get_bit_version(temp_reg, target_size));
    }

#include "eaglevm-core/codec/zydis_helper.h"

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_enter_ptr& cmd)
    {
        const asmb::code_label_ptr vm_enter = han_man->get_vm_enter();
        const asmb::code_label_ptr ret = asmb::code_label::create("vmenter_ret target");

        block->add(RECOMPILE(encode(m_push, ZLABEL(ret))));
        if (settings->relative_addressing)
        {
            block->add(RECOMPILE(encode(m_jmp, ZJMPR(vm_enter))));
        }
        else
        {
            block->add(RECOMPILE_CHUNK([=](uint64_t)
                {
                const uint64_t address = vm_enter->get_address();

                std::vector<enc::req> address_gen;
                address_gen.push_back(encode(m_push, ZIMMU(
                    static_cast<uint32_t>(address) >= 0x80000000 ?
                    static_cast<uint64_t>(address) | 0xFF'FF'FF'FF'00'00'00'00 :
                    static_cast<uint32_t>(address))));

                address_gen.push_back(encode(m_mov, ZMEMBD(rsp, 4, 4), ZIMMS(static_cast<uint32_t>(address >> 32))));
                address_gen.push_back(encode(m_ret));

                return codec::compile_queue(address_gen);
                }));
        }

        block->bind(ret);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_exit_ptr& cmd)
    {
        const asmb::code_label_ptr vm_exit = han_man->get_vm_exit();
        const asmb::code_label_ptr ret = asmb::code_label::create("vmexit_ret target");

        // mov VCSRET, ZLABEL(target)
        block->add(RECOMPILE(encode(m_mov, ZREG(VCSRET), ZLABEL(ret))));
        block->add(RECOMPILE(encode(m_jmp, ZJMPR(vm_exit))));
        block->bind(ret);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_dynamic_ptr& cmd)
    {
        const ir::encoder::encoder& encoder = cmd->get_encoder();
        ir::encoder::val_var_resolver resolver_func = [this](ir::encoder::val_variant& arg) -> reg
        {
            return std::visit([&, this]<typename val_type>(val_type&& type_arg)
            {
                if constexpr (std::is_same_v<std::decay_t<val_type>, ir::discrete_store_ptr>)
                {
                    const ir::discrete_store_ptr& store = type_arg;
                    reg_64_container->assign(store);

                    return store->get_store_register();
                }
                else if constexpr (std::is_same_v<std::decay_t<val_type>, ir::reg_vm>)
                {
                    const ir::reg_vm& store = type_arg;
                    return reg_vm_to_register(store);
                }
                else if constexpr (std::is_same_v<std::decay_t<val_type>, uint64_t>)
                {
                    // we should not be translating uint64_t
                    VM_ASSERT("unknown val_type translated");
                }
                else
                {
                    VM_ASSERT("unknown val_type translated");
                    return none;
                }
            }, arg);
        };

        block->add(encoder.create(resolver_func));
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd)
    {
        block->add(cmd->get_request());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_flags_load& cmd)
    {
        reg flag_reg = reg_man->get_vm_reg(register_manager::index_vflags);

        scope_register_manager scope = reg_64_container->create_scope();
        reg temp = scope.reserve();

        const uint32_t target_mask = ir::cmd_flags_load::get_flag_index(cmd.get_flag());
        block->add({
            encode(m_mov, ZREG(temp), ZREG(flag_reg)),
            encode(m_and, ZREG(temp), ZIMMU(target_mask)),
        });

        call_push(block, temp);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_jmp_ptr& cmd)
    {
        scope_register_manager scope = reg_64_container->create_scope();
        reg temp = scope.reserve();

        call_pop(block, temp);
        block->add(encode(m_jmp, ZREG(temp)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_and_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_and, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_or_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_or, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_xor_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_xor, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shl_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_and, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shr_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_shr, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_add_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_add, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sub_ptr& cmd)
    {
        handle_generic_logic_cmd(block, m_sub, cmd->get_size());
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cmp_ptr& cmd)
    {
        scope_register_manager scope = reg_64_container->create_scope();
        std::array<reg, 4> vals = scope.reserve<4>();
        for (auto& val : vals) val = get_bit_version(val, to_reg_size(cmd->get_size()));

        call_pop(block, vals[1]);
        call_pop(block, vals[0]);

        for (auto flag : ir::vm_flags_list)
        {
            const auto idx = ir::cmd_flags_load::get_flag_index(flag);
            auto build_cmp = [&](mnemonic mnemonic)
            {
                block->add({
                    // mask off EQ flag
                    encode(m_mov, ZREG(vals[2]), ZIMMU(~(1ull << idx))),
                    encode(m_and, ZREG(VFLAGS), ZREG(vals[2])),

                    // clear target register and CMP initial values
                    encode(m_xor, ZREG(vals[2]), ZREG(vals[2])),
                    encode(m_cmp, ZREG(vals[0]), ZREG(vals[1])),
                    encode(m_mov, ZREG(vals[3]), ZIMMU(1ull << idx)),
                    encode(mnemonic, ZREG(vals[2]), ZREG(vals[3])),
                    encode(m_or, ZREG(VFLAGS), ZREG(vals[2]))
                });
            };

            switch (flag)
            {
                case ir::vm_flags::eq:
                {
                    // set eq = zf
                    build_cmp(m_cmovz);
                    break;
                }
                case ir::vm_flags::le:
                {
                    // set le = SF <> OF
                    build_cmp(m_cmovl);
                    break;
                }
                case ir::vm_flags::ge:
                {
                    // set ge = ZF = 0 and SF = OF
                    build_cmp(m_cmovnle);
                    break;
                }
            }
        }
    }

    std::vector<asmb::code_container_ptr> machine::create_handlers()
    {
        return han_man->build_handlers();
    }

    void machine::call_push(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared) const
    {
        reg pushing_register;
        const reg_size size = to_reg_size(shared->get_store_size());

        if (!settings->randomize_working_register)
        {
            pushing_register = han_man->get_push_working_register();

            reg target_returning = get_bit_version(pushing_register, size);
            reg target_store = get_bit_version(shared->get_store_register(), size);
            block->add(encode(m_mov, ZREG(target_returning), ZREG(target_store)));
        }
        else
        {
            pushing_register = shared->get_store_register();
        }

        han_man->call_vm_handler(block, han_man->get_push(pushing_register, size));
    }

    void machine::call_push(const asmb::code_container_ptr& block, const reg target_reg) const
    {
        reg pushing_register;
        const reg_size size = get_reg_size(target_reg);

        if (!settings->randomize_working_register)
        {
            pushing_register = han_man->get_push_working_register();

            reg target_returning = get_bit_version(pushing_register, size);
            reg target_store = get_bit_version(target_reg, size);
            block->add(encode(m_mov, ZREG(target_returning), ZREG(target_store)));
        }
        else
        {
            pushing_register = get_bit_version(target_reg, bit_64);
        }

        han_man->call_vm_handler(block, han_man->get_push(pushing_register, size));
    }

    void machine::call_pop(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared) const
    {
        const reg_size size = to_reg_size(shared->get_store_size());
        if (!settings->randomize_working_register)
        {
            const reg returning_reg = han_man->get_pop_working_register();

            reg target_returning = get_bit_version(returning_reg, size);
            reg target_store = get_bit_version(shared->get_store_register(), size);

            han_man->call_vm_handler(block, han_man->get_pop(returning_reg, size));
            block->add(encode(m_mov, ZREG(target_store), ZREG(target_returning)));
        }
        else
        {
            han_man->call_vm_handler(block, han_man->get_pop(shared->get_store_register(), size));
        }
    }

    void machine::call_pop(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared, const reg_size size) const
    {
        VM_ASSERT(to_reg_size(shared->get_store_size()) >= size, "pop size cannot be greater than store size");
        if (!settings->randomize_working_register)
        {
            const reg returning_reg = han_man->get_pop_working_register();

            reg target_returning = get_bit_version(returning_reg, size);
            reg target_store = get_bit_version(shared->get_store_register(), size);

            han_man->call_vm_handler(block, han_man->get_pop(returning_reg, size));
            block->add(encode(m_mov, ZREG(target_store), ZREG(target_returning)));
        }
        else
        {
            han_man->call_vm_handler(block, han_man->get_pop(shared->get_store_register(), size));
        }
    }

    void machine::call_pop(const asmb::code_container_ptr& block, const reg target_reg) const
    {
        const reg_size size = get_reg_size(target_reg);
        if (!settings->randomize_working_register)
        {
            const reg returning_reg = han_man->get_pop_working_register();

            reg target_returning = get_bit_version(returning_reg, size);
            reg target_store = get_bit_version(target_reg, size);

            han_man->call_vm_handler(block, han_man->get_pop(returning_reg, size));
            block->add(encode(m_mov, ZREG(target_store), ZREG(target_returning)));
        }
        else
        {
            han_man->call_vm_handler(block, han_man->get_pop(target_reg, size));
        }
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

    void machine::handle_generic_logic_cmd(const asmb::code_container_ptr& block, const mnemonic command, const ir::ir_size size)
    {
        scope_register_manager scope = reg_64_container->create_scope();
        std::array<reg, 2> vals = scope.reserve<2>();
        for (auto& val : vals) val = get_bit_version(val, to_reg_size(size));

        call_pop(block, vals[1]);
        call_pop(block, vals[0]);

        block->add(encode(command, ZREG(vals[0]), ZREG(vals[1])));

        call_push(block, vals[0]);
    }
}
