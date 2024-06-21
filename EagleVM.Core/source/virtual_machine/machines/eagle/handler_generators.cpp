#include <utility>
#include <ranges>

#include "eaglevm-core/virtual_machine/machines/eagle/handler_manager.h"
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"

#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"

#define VIP         regs->get_vm_reg(register_manager::index_vip)
#define VSP         regs->get_vm_reg(register_manager::index_vsp)
#define VREGS       regs->get_vm_reg(register_manager::index_vregs)
#define VCS         regs->get_vm_reg(register_manager::index_vcs)
#define VCSRET      regs->get_vm_reg(register_manager::index_vcsret)
#define VBASE       regs->get_vm_reg(register_manager::index_vbase)

using namespace eagle::codec;

namespace eagle::virt::eg
{
    std::vector<asmb::code_container_ptr> handler_manager::build_handlers()
    {
        std::vector<asmb::code_container_ptr> handlers;

        handlers.push_back(build_vm_enter());
        handlers.push_back(build_vm_exit());

        handlers.push_back(build_rflags_load());
        handlers.push_back(build_rflags_store());

        handlers.append_range(build_instruction_handlers());

        handlers.append_range(build_pop());
        handlers.append_range(build_push());

        for (auto& [variant_pairs] : register_load_handlers | std::views::values)
            for (auto& container : variant_pairs | std::views::keys)
                handlers.push_back(std::get<0>(container));

        for (auto& [variant_pairs] : register_store_handlers | std::views::values)
            for (auto& container : variant_pairs | std::views::keys)
                handlers.push_back(std::get<0>(container));

        return handlers;
    }

    asmb::code_container_ptr handler_manager::build_vm_enter()
    {
        auto [container, label] = vm_enter.get_pair();
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

        // mov VBASE, 0x14000000 ; load base
        const asmb::code_label_ptr rel_label = asmb::code_label::create();

        // mov temp, rel_offset_to_virtual_base
        container->bind(rel_label);
        container->add(RECOMPILE(encode(m_lea, ZREG(VBASE), ZMEMBD(rip, 0, 8))));
        container->add(RECOMPILE(encode(m_mov, ZREG(temp), ZIMMS(-rel_label->get_relative_address()))));
        container->add(RECOMPILE(encode(m_lea, ZREG(VBASE), ZMEMBI(VBASE, temp, 1, TOB(bit_64)))));

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp (+1 because we pushed an rva)
        // mov VSP, VTEMP
        container->add({
            encode(m_lea, ZREG(temp), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead + 1), TOB(bit_64))),
            encode(m_mov, ZREG(VSP), ZREG(temp)),
        });

        // setup register mampings
        std::array<reg, 16> gprs = regs->get_gpr64_regs();
        if (settings->shuffle_push_order)
            std::ranges::shuffle(gprs, util::ran_device::get().gen);

        for (const auto& gpr : gprs)
        {
            reg target_reg;
            if (settings->single_register_handlers)
                target_reg = regs->get_reserved_temp(2);
            else
                target_reg = regs_64_context->get_any();

            auto [disp, _] = regs->get_stack_displacement(gpr);

            container->add(encode(m_mov, ZREG(target_reg), ZMEMBD(VREGS, disp, 8)));
            call_vm_handler(container, std::get<0>(store_register(gpr, target_reg)));
        }

        create_vm_return(container);
        return container;
    }

    asmb::code_container_ptr handler_manager::build_vm_exit()
    {
        auto [container, label] = vm_exit.get_pair();
        container->bind(label);

        reg temp = regs_64_context->get_any();

        // we need to place the target RSP after all the pops
        // lea VTEMP, [VREGS + vm_stack_regs]
        // mov [VTEMP], VSP
        container->add({
            encode(m_lea, ZREG(temp), ZMEMBD(VREGS, 8 * vm_stack_regs, TOB(bit_64))),
            encode(m_mov, ZMEMBD(temp, 0, 8), ZREG(VSP))
        });

        // we also need to setup an RIP to return to main program execution
        // we will place that after the RSP
        container->add(RECOMPILE(encode(m_mov, ZREG(VIP), ZREG(VBASE))));

        container->add(encode(m_lea, ZREG(VIP), ZMEMBI(VIP, VCSRET, 1, TOB(bit_64))));
        container->add(encode(m_mov, ZMEMBD(VSP, -8, 8), ZREG(VIP)));

        // mov rsp, VREGS
        container->add(encode(m_mov, ZREG(rsp), ZREG(VREGS)));

        // restore context
        std::array<reg, 16> gprs = regs->get_gpr64_regs();
        if (settings->shuffle_push_order)
            std::ranges::shuffle(gprs, util::ran_device::get().gen);

        for (const auto& gpr : gprs)
        {
            reg target_reg;
            if (settings->single_register_handlers)
                target_reg = regs->get_reserved_temp(2);
            else
                target_reg = regs_64_context->get_any();

            auto [disp, _] = regs->get_stack_displacement(gpr);

            container->add(encode(m_xor, ZREG(target_reg), ZREG(target_reg)));

            call_vm_handler(container, std::get<0>(load_register(gpr, target_reg)));
            container->add(encode(m_mov, ZMEMBD(VREGS, disp, 8), ZREG(target_reg)));
        }

        //pop r0-r15 to stack
        regs->enumerate([&container](auto reg)
        {
            if (reg == ZYDIS_REGISTER_RSP)
            {
                container->add(encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, TOB(bit_64))));
            }
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

    asmb::code_container_ptr handler_manager::build_rflags_load()
    {
        auto [container, label] = vm_rflags_load.get_pair();
        container->bind(label);

        container->add({
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -8, TOB(bit_64))),
            encode(m_popfq),
        });

        create_vm_return(container);
        return container;
    }

    asmb::code_container_ptr handler_manager::build_rflags_store()
    {
        auto [container, label] = vm_rflags_store.get_pair();
        container->bind(label);

        container->add({
            encode(m_pushfq),
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, TOB(bit_64))),
        });

        create_vm_return(container);
        return container;
    }

    std::vector<asmb::code_container_ptr> handler_manager::build_push()
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (auto& [target_temp, variant_handler] : vm_push)
        {
            auto& [container, label] = variant_handler;
            container->bind(label);

            const reg_size reg_size = get_reg_size(target_temp);
            container->add({
                encode(m_lea, ZREG(VSP), ZMEMBD(VSP, -TOB(reg_size), TOB(bit_64))),
                encode(m_mov, ZMEMBD(VSP, 0, TOB(reg_size)), ZREG(target_temp))
            });

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
    }

    std::vector<asmb::code_container_ptr> handler_manager::build_pop()
    {
        std::vector<asmb::code_container_ptr> context_stores;
        for (auto& [target_temp, variant_handler] : vm_pop)
        {
            auto& [container, label] = variant_handler;
            container->bind(label);

            const reg_size reg_size = get_reg_size(target_temp);
            container->add({
                encode(m_mov, ZREG(target_temp), ZMEMBD(VSP, 0, TOB(reg_size))),
                encode(m_lea, ZREG(VSP), ZMEMBD(VSP, TOB(reg_size), 8)),
            });

            create_vm_return(container);
            context_stores.push_back(container);
        }

        return context_stores;
    }

    std::vector<asmb::code_container_ptr> handler_manager::build_instruction_handlers()
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

            const std::shared_ptr<machine> machine = machine_inst.lock();
            const asmb::code_container_ptr handler = machine->lift_block(ir_block);
            handler->bind_start(label);

            create_vm_return(handler);
            container.push_back(handler);
        }

        return container;
    }
}
