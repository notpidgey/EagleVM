#include <utility>

#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/util/random.h"

#include "eaglevm-core/virtual_machine/machines/util.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/machine.h"

#define VIP         inst_regs->get_reg(I_VIP)
#define VSP         inst_regs->get_reg(I_VSP)
#define VREGS       inst_regs->get_reg(I_VREGS)
#define VCS         inst_regs->get_reg(I_VCALLSTACK)
#define VCSRET      inst_regs->get_reg(I_VCSRET)
#define VBASE       inst_regs->get_reg(I_VBASE)

#define VTEMP       inst_regs->get_reg_temp(0)
#define VTEMP2      inst_regs->get_reg_temp(1)
#define VTEMPX(x)   inst_regs->get_reg_temp(x)

using namespace eagle::codec;

namespace eagle::virt::pidg
{
    inst_handlers::inst_handlers(machine_ptr machine, vm_inst_regs_ptr push_order, settings_ptr settings)
        : settings(std::move(settings)), machine(std::move(machine)), inst_regs(std::move(push_order))
    {
        vm_overhead = 8 * 100;
        vm_stack_regs = 17;
        vm_call_stack = 3;
    }

    void inst_handlers::randomize_constants()
    {
        std::uniform_int_distribution<uint64_t> overhead_distribution(1000, 2000);
        const uint16_t random_overhead_bytes = util::ran_device::get().gen_dist(overhead_distribution);

        std::uniform_int_distribution<uint64_t> call_stack_distribution(3, 10);
        const uint16_t random_callstack_bytes = util::ran_device::get().gen_dist(call_stack_distribution);

        vm_overhead = 8 * random_overhead_bytes;
        vm_stack_regs = 16 + 1; // 16 GPR + RFLAGS
        vm_call_stack = random_callstack_bytes;
    }

    asmb::code_label_ptr inst_handlers::get_vm_enter(const bool reference)
    {
        if (reference)
            vm_enter.tagged = true;

        return vm_enter.label;
    }

    asmb::code_container_ptr inst_handlers::build_vm_enter()
    {
        vm_enter.code = asmb::code_container::create();
        vm_enter.code->bind(vm_enter.label);

        asmb::code_container_ptr container = vm_enter.code;

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
        inst_regs->enumerate(
            [&container](short reg)
            {
                container->add(encode(m_push, ZREG(reg)));
            });

        // mov VSP, rsp         ; begin virtualization by setting VSP to rsp
        // mov VREGS, VSP       ; set VREGS to currently pushed stack items
        // mov VCS, VSP         ; set VCALLSTACK to current stack top

        // lea rsp, [rsp + stack_regs + 1] ; this allows us to move the stack pointer in such a way that pushfq overwrite rflags on the stack

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead))] ; load the address of where return address is located
        // mov VTEMP, [VTEMP]   ; load actual value into VTEMP
        // lea VCS, [VCS - 8]   ; allocate space to place return address
        // mov [VCS], VTEMP     ; put return address onto call stack

        container->add({
            encode(m_mov, ZREG(VSP), ZREG(rsp)),
            encode(m_mov, ZREG(VREGS), ZREG(VSP)),
            encode(m_mov, ZREG(VCS), ZREG(VSP)),

            encode(m_lea, ZREG(rsp), ZMEMBD(VREGS, 8 * vm_stack_regs, TOB(bit_64))),

            encode(m_lea, ZREG(VTEMP), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead), TOB(bit_64))),
            encode(m_mov, ZREG(VTEMP), ZMEMBD(VTEMP, 0, TOB(bit_64))),
            encode(m_lea, ZREG(VCS), ZMEMBD(VCS, -8, TOB(bit_64))),
            encode(m_mov, ZMEMBD(VCS, 0, 8), ZREG(VTEMP)),
        });

        // lea VIP, [0x14000000]    ; load base
        const asmb::code_label_ptr rel_label = asmb::code_label::create();
        container->bind(rel_label);
        container->add(RECOMPILE(encode(m_lea, ZREG(VBASE), ZMEMBD(rip, -rel_label->get_address(), TOB(bit_64)))));

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp (+1 because we pushed an rva)
        // mov VSP, VTEMP
        container->add({
            encode(m_lea, ZREG(VTEMP), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead + 1), TOB(bit_64))),
            encode(m_mov, ZREG(VSP), ZREG(VTEMP)),
        });

        create_vm_return(container);
        return container;
    }

    asmb::code_label_ptr inst_handlers::get_vm_exit(const bool reference)
    {
        if (reference)
            vm_exit.tagged = true;

        return vm_exit.label;
    }

    asmb::code_container_ptr inst_handlers::build_vm_exit()
    {
        vm_exit.code = asmb::code_container::create();
        vm_exit.code->bind(vm_exit.label);

        asmb::code_container_ptr container = vm_exit.code;

        // we need to place the target RSP after all the pops
        // lea VTEMP, [VREGS + vm_stack_regs]
        // mov [VTEMP], VSP
        container->add({
            encode(m_lea, ZREG(VTEMP), ZMEMBD(VREGS, 8 * vm_stack_regs, TOB(bit_64))),
            encode(m_mov, ZMEMBD(VTEMP, 0, 8), ZREG(VSP))
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
        inst_regs->enumerate([&container](auto reg)
        {
            if (reg == ZYDIS_REGISTER_RSP || reg == ZYDIS_REGISTER_RIP)
                container->add(encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, TOB(bit_64))));
            else
                container->add(encode(m_pop, ZREG(reg)));
        }, true);

        //popfq
        {
            container->add(encode(m_popfq));
        }

        // the rsp that we setup earlier before popping all the regs
        container->add(encode(m_pop, ZREG(rsp)));
        container->add(encode(m_jmp, ZMEMBD(rsp, -8, TOB(bit_64))));

        return vm_exit.code;
    }

    asmb::code_label_ptr inst_handlers::get_rlfags_load(const bool reference)
    {
        if (reference)
            vm_rflags_load.tagged = true;

        return vm_rflags_load.label;
    }

    asmb::code_container_ptr inst_handlers::build_rflags_load()
    {
        vm_rflags_load.code = asmb::code_container::create();

        const asmb::code_container_ptr container = vm_rflags_load.code;
        container->bind(vm_rflags_load.label);
        container->add({
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -8, TOB(bit_64))),
            encode(m_popfq),
        });

        create_vm_return(container);
        return vm_rflags_load.code;
    }

    asmb::code_label_ptr inst_handlers::get_rflags_store(const bool reference)
    {
        if (reference)
            vm_rflags_save.tagged = true;

        return vm_rflags_save.label;
    }

    asmb::code_container_ptr inst_handlers::build_rflags_save()
    {
        vm_rflags_save.code = asmb::code_container::create();

        const asmb::code_container_ptr container = vm_rflags_save.code;
        container->bind(vm_rflags_save.label);
        container->add({
            encode(m_pushfq),
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, TOB(bit_64))),
        });

        create_vm_return(container);
        return vm_rflags_save.code;
    }

    asmb::code_label_ptr inst_handlers::get_context_load(const reg_size size)
    {
        switch (size)
        {
            case bit_64:
            {
                auto& ctx = vm_load[0];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_32:
            {
                auto& ctx = vm_load[1];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_16:
            {
                auto& ctx = vm_load[2];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_8:
            {
                auto& ctx = vm_load[3];
                ctx.tagged = true;
                return ctx.label;
            }
            default:
                assert("reached invalid load size");
        }

        return nullptr;
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_context_load()
    {
        std::vector<asmb::code_container_ptr> context_loads;
        for (uint8_t i = 0; i < 4; i++)
        {
            const tagged_vm_handler& handler = vm_load[i];
            if (!handler.tagged)
                continue;

            asmb::code_container_ptr container = handler.code;
            container->bind(handler.label);

            const reg_size reg_size = load_store_index_size(i);
            reg target_temp = get_bit_version(VTEMP, get_gpr_class_from_size(reg_size));

            container->add(encode(m_mov, ZREG(target_temp), ZMEMBI(VREGS, VTEMP, 1, TOB(reg_size))));
            call_vm_handler(container, get_push(reg_size));

            create_vm_return(container);
            context_loads.push_back(container);
        }

        return context_loads;
    }

    asmb::code_label_ptr inst_handlers::get_context_store(const reg_size size)
    {
        switch (size)
        {
            case bit_64:
            {
                auto& ctx = vm_store[0];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_32:
            {
                auto& ctx = vm_store[1];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_16:
            {
                auto& ctx = vm_store[2];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_8:
            {
                auto& ctx = vm_store[3];
                ctx.tagged = true;
                return ctx.label;
            }
            default:
                assert("reached invalid store size");
        }

        return nullptr;
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_context_store()
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (uint8_t i = 0; i < 4; i++)
        {
            const tagged_vm_handler& handler = vm_store[i];
            if (!handler.tagged)
                continue;

            asmb::code_container_ptr container = handler.code;
            container->bind(handler.label);

            const reg_size reg_size = load_store_index_size(i);
            reg target_temp = get_bit_version(VTEMP, get_gpr_class_from_size(reg_size));

            if (reg_size == bit_32)
            {
                // we have to clear upper 32 bits of target register here
                container->add(encode(m_lea, ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, TOB(bit_64))));
                call_vm_handler(container, get_pop(reg_size));
                container->add(encode(m_mov, ZMEMBD(VTEMP2, 0, TOB(bit_64)), ZIMMS(0)));
                container->add(encode(m_mov, ZMEMBD(VTEMP2, 0, TOB(reg_size)), ZREG(target_temp)));
            }
            else
            {
                container->add(encode(m_lea, ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, TOB(bit_64))));
                call_vm_handler(container, get_pop(reg_size));
                container->add(encode(m_mov, ZMEMBD(VTEMP2, 0, TOB(reg_size)), ZREG(target_temp)));
            }

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
    }

    asmb::code_label_ptr inst_handlers::get_push(reg_size size)
    {
        switch (size)
        {
            case bit_64:
            {
                auto& ctx = vm_push[0];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_32:
            {
                auto& ctx = vm_push[1];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_16:
            {
                auto& ctx = vm_push[2];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_8:
            {
                auto& ctx = vm_push[3];
                ctx.tagged = true;
                return ctx.label;
            }
            default:
                assert("reached invalid push size");
        }

        return nullptr;
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_push() const
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (uint8_t i = 0; i < 4; i++)
        {
            const tagged_vm_handler& handler = vm_push[i];
            if (!handler.tagged)
                continue;

            const asmb::code_container_ptr container = handler.code;
            container->bind(handler.label);

            const reg_size reg_size = load_store_index_size(i);
            const uint16_t reg_size_bytes = reg_size / 8;

            reg target_temp = get_bit_version(VTEMP, get_gpr_class_from_size(reg_size));
            container->add({
                encode(m_lea, ZREG(VSP), ZMEMBD(VSP, -reg_size_bytes, TOB(bit_64))),
                encode(m_mov, ZMEMBD(VSP, 0, TOB(reg_size)), ZREG(target_temp))
            });

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
    }

    asmb::code_label_ptr inst_handlers::get_pop(reg_size size)
    {
        switch (size)
        {
            case bit_64:
            {
                auto& ctx = vm_pop[0];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_32:
            {
                auto& ctx = vm_pop[1];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_16:
            {
                auto& ctx = vm_pop[2];
                ctx.tagged = true;
                return ctx.label;
            }
            case bit_8:
            {
                auto& ctx = vm_pop[3];
                ctx.tagged = true;
                return ctx.label;
            }
            default:
                assert("reached invalid pop size");
        }

        return nullptr;
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_pop() const
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (uint8_t i = 0; i < 4; i++)
        {
            const tagged_vm_handler& handler = vm_pop[i];
            if (!handler.tagged)
                continue;

            const asmb::code_container_ptr container = handler.code;
            container->bind(handler.label);

            const reg_size reg_size = load_store_index_size(i);

            reg target_temp = get_bit_version(VTEMP, get_gpr_class_from_size(reg_size));
            container->add({
                encode(m_mov, ZREG(target_temp), ZMEMBD(VSP, 0, TOB(reg_size))),
                encode(m_lea, ZREG(VSP), ZMEMBD(VSP, TOB(reg_size), 8)),
            });

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
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

    std::vector<asmb::code_container_ptr> inst_handlers::build_instruction_handlers()
    {
        std::vector<asmb::code_container_ptr> container;
        for (const auto& [key, label] : tagged_instruction_handlers)
        {
            auto [mnemonic, handler_id] = key;

            const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];
            ir::ir_insts handler_ir = target_mnemonic->gen_handler(handler_id);

            // todo: walk each block and guarantee that discrete_store variables only use vtemps we want
            ir::block_ptr ir_block = std::make_shared<ir::block_ir>();
            ir_block->add_command(handler_ir);

            const asmb::code_container_ptr handler = machine->lift_block(ir_block);
            handler->bind_start(label);

            create_vm_return(handler);
            container.push_back(handler);
        }

        return container;
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_handlers()
    {
        std::vector<asmb::code_container_ptr> handlers;
        if (vm_enter.tagged) handlers.push_back(build_vm_enter());
        if (vm_exit.tagged) handlers.push_back(build_vm_exit());
        if (vm_rflags_load.tagged) handlers.push_back(build_rflags_load());
        if (vm_rflags_save.tagged) handlers.push_back(build_rflags_save());

        handlers.append_range(build_instruction_handlers());

        handlers.append_range(build_context_load());
        handlers.append_range(build_context_store());

        handlers.append_range(build_push());
        handlers.append_range(build_pop());

        return handlers;
    }

    void inst_handlers::call_vm_handler(const asmb::code_container_ptr& code, const asmb::code_label_ptr& target) const
    {
        assert(target != nullptr, "target cannot be an invalid code label");
        assert(code != nullptr, "code cannot be an invalid code label");

        // todo: on debug verify that the target is a valid handler

        const asmb::code_label_ptr return_label = asmb::code_label::create("caller return");

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        code->add(encode(m_lea, ZREG(VCS), ZMEMBD(VCS, -8, TOB(bit_64))));
        code->add(RECOMPILE(encode(m_mov, ZMEMBD(VCS, 0, TOB(bit_64)), ZLABEL(return_label))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        code->add(RECOMPILE(encode(m_lea, ZREG(VIP), ZMEMBD(VBASE, target->get_address(), TOB(bit_64)))));
        code->add(encode(m_jmp, ZREG(VIP)));

        // execution after VM handler should end up here
        code->bind(return_label);
    }

    void inst_handlers::create_vm_return(const asmb::code_container_ptr& container) const
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

    reg_size inst_handlers::load_store_index_size(const uint8_t index)
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
