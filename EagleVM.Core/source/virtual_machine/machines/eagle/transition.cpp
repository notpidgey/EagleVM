#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

#include <unordered_set>

#include "eaglevm-core/virtual_machine/machines/util.h"
#include "eaglevm-core/virtual_machine/machines/eagle/handler.h"
#include "eaglevm-core/virtual_machine/machines/eagle/loader.h"

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

namespace eagle::virt::eg
{
    using namespace codec;
    using namespace codec::encoder;

    constexpr int32_t vm_overhead = 8 * 100;
    constexpr int32_t vm_stack_regs = 17 + 16 * 2;
    constexpr int32_t vm_call_stack = 3;

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_enter_ptr& cmd)
    {
        encode_builder& builder = *block;

        // TODO: this is a temporary fix before i add stack overrun checks
        // we allocate the registers for the virtual machine 20 pushes after the current stack

        // reserve VM call stack
        // reserve VM stack
        builder.make(m_lea, reg_op(rsp), mem_op(rsp, -(8 * vm_overhead), TOB(bit_64)));

        // pushfq
        {
            builder.make(m_pushfq);
        }

        // push r0-r15 to stack
        regs->enumerate(
            [&](const reg reg)
            {
                if (get_reg_class(reg) == xmm_128)
                {
                    builder.make(m_lea, reg_op(rsp), mem_op(rsp, -16, TOB(bit_64)))
                           .make(m_movq, mem_op(rsp, 0, TOB(bit_128)), reg_op(reg));
                }
                else
                {
                    builder.make(m_push, reg_op(reg));
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

        const reg temp = regs->get_reserved_temp(0);
        builder.make(m_mov, reg_op(VSP), reg_op(rsp))
               .make(m_mov, reg_op(VREGS), reg_op(VSP))
               .make(m_mov, reg_op(VCS), reg_op(VSP))
               .make(m_lea, reg_op(rsp), mem_op(VREGS, 8 * vm_stack_regs, bit_64));

        // .make(m_lea, reg_op(temp), mem_op(VSP, 8 * (vm_stack_regs + vm_overhead), bit_64))
        // .make(m_mov, reg_op(temp), mem_op(temp, 0, bit_64))
        // .make(m_lea, reg_op(VCS), mem_op(VCS, -8, bit_64))
        // .make(m_mov, mem_op(VCS, 0, 8), reg_op(temp));

        // mov VBASE, 0x14000000 ; load base
        const asmb::code_label_ptr rel_label = asmb::code_label::create();

        // mov temp, rel_offset_to_virtual_base
        builder.label(rel_label);
        builder.make(m_lea, reg_op(VBASE), mem_op(rip, 0, bit_64))
               .make(m_mov, reg_op(temp), imm_label_operand(rel_label, false, true))
               .make(m_lea, reg_op(VBASE), mem_op(VBASE, temp, 1, 0, bit_64));

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp (+1 because we pushed a rva)
        // mov VSP, VTEMP
        builder.make(m_lea, reg_op(temp), mem_op(VSP, 8 * (vm_stack_regs + vm_overhead), bit_64))
               .make(m_mov, reg_op(VSP), reg_op(temp));

        // setup register mappings
        std::array<reg, 16> gprs = register_manager::get_gpr64_regs();
        if (settings->shuffle_push_order) std::ranges::shuffle(gprs, util::ran_device::get().gen);

        for (const reg& gpr : gprs)
        {
            auto [displacement, _] = regs->get_stack_displacement(gpr);

            scope_register_manager scope = reg_64_container->create_scope();
            const reg target_temp = scope.reserve();

            // push the reg onto the stack
            builder.make(m_mov, reg_op(target_temp), mem_op(VREGS, displacement, 8))
                   .make(m_sub, reg_op(VSP), imm_op(bit_64))
                   .make(m_mov, mem_op(VSP, 0, bit_64), reg_op(target_temp));

            // store it however we intend to
            handle_cmd(block, std::make_shared<ir::cmd_context_store>(gpr));
        }

        builder.make(m_nop);
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_exit_ptr& cmd)
    {
        encode_builder& builder = *block;

        builder.make(m_nop);

        // restore context
        std::array<reg, 16> gprs = register_manager::get_gpr64_regs();
        if (settings->shuffle_push_order)
            std::ranges::shuffle(gprs, util::ran_device::get().gen);

        for (const auto& gpr : gprs)
        {
            auto [displacement, _] = regs->get_stack_displacement(gpr);

            scope_register_manager scope = reg_64_container->create_scope();
            const reg target_temp = scope.reserve();

            handle_cmd(block, std::make_shared<ir::cmd_context_load>(gpr));
            builder.make(m_mov, reg_op(target_temp), mem_op(VSP, 0, bit_64))
                   .make(m_add, reg_op(VSP), imm_op(bit_64))
                   .make(m_mov, mem_op(VREGS, displacement, bit_64), reg_op(target_temp));
        }

        // push exits
        handle_cmd(block, std::make_shared<ir::cmd_push>(std::visit([](auto&& arg) -> ir::push_v {
            return ir::push_v{std::forward<decltype(arg)>(arg)};
        }, cmd->get_exit()), ir::ir_size::bit_64));

        builder.make(m_mov, reg_op(VCSRET), mem_op(VSP, 0, bit_64))
               .make(m_add, reg_op(VSP), imm_op(bit_64));

        const reg temp = reg_64_container->get_any();

        // we need to place the target RSP after all the pops
        // lea VTEMP, [VREGS + vm_stack_regs]
        // mov [VTEMP], VSP
        builder.make(m_lea, reg_op(temp), mem_op(VREGS, 8 * vm_stack_regs, bit_64))
               .make(m_mov, mem_op(temp, 0, bit_64), reg_op(VSP));

        // we also need to setup an RIP to return to main program execution
        // we will place that after the RSP
        builder.make(m_mov, reg_op(VIP), reg_op(VBASE))
               .make(m_lea, reg_op(VIP), mem_op(VIP, VCSRET, 1, 0, bit_64))
               .make(m_mov, mem_op(VSP, -8, bit_64), reg_op(VIP));

        // mov rsp, VREGS
        builder.make(m_mov, reg_op(rsp), reg_op(VREGS));

        //pop r0-r15 to stack
        regs->enumerate([&](auto reg)
        {
            if (reg == ZYDIS_REGISTER_RSP)
            {
                builder.make(m_lea, reg_op(rsp), mem_op(rsp, 8, bit_64));
            }
            else
            {
                if (get_reg_class(reg) == xmm_128)
                {
                    builder.make(m_movdqu, reg_op(reg), mem_op(rsp, 0, bit_128))
                           .make(m_lea, reg_op(rsp), mem_op(rsp, 16, bit_64));
                }
                else
                {
                    builder.make(m_pop, reg_op(reg));
                }
            }
        }, true);

        // popfq
        {
            builder.make(m_popfq);
        }

        // the rsp that we set up earlier before popping all the regs
        builder.make(m_pop, reg_op(rsp))
               .make(m_jmp, mem_op(rsp, -8, bit_64));

        builder.make(m_nop);
    }
}
