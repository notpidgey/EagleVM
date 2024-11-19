#include "eaglevm-core/virtual_machine/machines/owl/machine.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"
#include "eaglevm-core/virtual_machine/machines/owl/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/owl/settings.h"

#include <unordered_set>

#include "eaglevm-core/virtual_machine/machines/util.h"
#include "eaglevm-core/virtual_machine/machines/owl/handler.h"
#include "eaglevm-core/virtual_machine/machines/owl/loader.h"

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

    constexpr int32_t vm_overhead = 8 * 100;
    constexpr int32_t vm_stack_regs = 17 + 16 * 2;
    constexpr int32_t vm_call_stack = 3;

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_enter_ptr& cmd)
    {
        encode_builder& builder = *block;

        // reserve VM call stack
        // reserve VM stack
        builder.make(m_lea, reg_op(rsp), mem_op(rsp, -(8 * vm_overhead), TOB(bit_64)));

        // pushfq
        {
            builder.make(m_pushfq);
        }

        // push ymm to stack
        regs->enumerate_ymm(
            [&](const reg reg)
            {
                builder.make(m_lea, reg_op(rsp), mem_op(rsp, -32, TOB(bit_64)))
                       .make(m_vmovdqu, mem_op(rsp, 0, TOB(bit_256)), reg_op(reg));
            });

        // setup register mappings
        std::array<reg, 16> gprs = register_manager::get_gpr64_regs();
        if (settings->shuffle_push_order) std::ranges::shuffle(gprs, util::ran_device::get().gen);

        reg temp_reg;

        for (auto& gpr : gprs)
        {
            // we store them in
            builder.make(m_mov, reg_op(temp_reg), reg_op(gpr));
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_exit_ptr& cmd)
    {
        encode_builder& builder = *block;

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

        // popfq
        {
            builder.make(m_popfq);
        }

        // the rsp that we set up earlier before popping all the regs
        builder.make(m_pop, reg_op(rsp))
               .make(m_jmp, mem_op(rsp, -8, bit_64));
    }
}
