#include <utility>

#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/machine.h"

#define VIP         inst_regs->get_reg(I_VIP)
#define VSP         inst_regs->get_reg(I_VSP)
#define VREGS       inst_regs->get_reg(I_VREGS)
#define VTEMP       inst_regs->get_reg_temp(0)
#define VTEMP2      inst_regs->get_reg_temp(1)
#define VTEMPX(x)   inst_regs->get_reg_temp(x)
#define VCS         inst_regs->get_reg(I_VCALLSTACK)
#define VCSRET      inst_regs->get_reg(I_VCSRET)
#define VBASE       inst_regs->get_reg(I_VBASE)

using namespace eagle::codec;

namespace eagle::virt::pidg
{
    inst_handlers::inst_handlers(machine_ptr machine, vm_inst_regs_ptr push_order)
        : machine(std::move(machine)), inst_regs(std::move(push_order))
    {
        vm_overhead = 8 * 2000;
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
        container->add(encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -(8 * vm_overhead), 8)));

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

            encode(m_lea, ZREG(rsp), ZMEMBD(VREGS, 8 * vm_stack_regs, 8)),

            encode(m_lea, ZREG(VTEMP), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead), 8)),
            encode(m_mov, ZREG(VTEMP), ZMEMBD(VTEMP, 0, 8)),
            encode(m_lea, ZREG(VCS), ZMEMBD(VCS, -8, 8)),
            encode(m_mov, ZMEMBD(VCS, 0, 8), ZREG(VTEMP)),
        });

        // lea VIP, [0x14000000]    ; load base
        const asmb::code_label_ptr rel_label = asmb::code_label::create();
        container->bind(rel_label);
        container->add(RECOMPILE(encode(m_lea, ZREG(VBASE), ZMEMBD(rip, -rel_label->get_address(), 8))));

        // lea VTEMP, [VSP + (8 * (stack_regs + vm_overhead) + 1)] ; load the address of the original rsp (+1 because we pushed an rva)
        // mov VSP, VTEMP
        container->add({
            encode(m_lea, ZREG(VTEMP), ZMEMBD(VSP, 8 * (vm_stack_regs + vm_overhead + 1), 8)),
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
            encode(m_lea, ZREG(VTEMP), ZMEMBD(VREGS, 8 * vm_stack_regs, 8)),
            encode(m_mov, ZMEMBD(VTEMP, 0, 8), ZREG(VSP))
        });

        // we also need to setup an RIP to return to main program execution
        // we will place that after the RSP
        const asmb::code_label_ptr rel_label = asmb::code_label::create();
        container->bind(rel_label);
        container->add(RECOMPILE(encode(m_lea, ZREG(VIP), ZMEMBD(rip, -rel_label->get_address(), 8))));
        container->add(encode(m_lea, ZREG(VIP), ZMEMBI(VIP, VCSRET, 1, 8)));
        container->add(encode(m_mov, ZMEMBD(VSP, -8, 8), ZREG(VIP)));

        // mov rsp, VREGS
        container->add(encode(m_mov, ZREG(rsp), ZREG(VREGS)));

        //pop r0-r15 to stack
        inst_regs->enumerate([&container](auto reg)
        {
            if (reg == ZYDIS_REGISTER_RSP || reg == ZYDIS_REGISTER_RIP)
                container->add(encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, 8)));
            else
                container->add(encode(m_pop, ZREG(reg)));
        }, true);

        //popfq
        {
            container->add(encode(m_popfq));
        }

        // the rsp that we setup earlier before popping all the regs
        container->add(encode(m_pop, ZREG(rsp)));
        container->add(encode(m_jmp, ZMEMBD(rsp, -8, 8)));

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
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, -8, 8)),
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
            encode(m_lea, ZREG(rsp), ZMEMBD(rsp, 8, 8)),
        });

        create_vm_return(container);
        return vm_rflags_save.code;
    }

    asmb::code_label_ptr inst_handlers::get_context_load(const reg_size size)
    {
        switch (size)
        {
            case bit_64:
                return vm_load[0].label;
            case bit_32:
                return vm_load[1].label;
            case bit_16:
                return vm_load[2].label;
            case bit_8:
                return vm_load[3].label;
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
            if(!handler.tagged)
                continue;

            asmb::code_container_ptr container = handler.code;

            const reg_size reg_size = load_store_index_size(i);
            reg target_temp = get_bit_version(VTEMP, get_gpr_class_from_size(reg_size));

            container->add(encode(m_mov, ZREG(target_temp), ZMEMBI(VREGS, VTEMP, 1, reg_size)));
            call_vm_handler(container, get_instruction_handler(m_push, 1, reg_size));

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
                return vm_store[0].label;
            case bit_32:
                return vm_store[1].label;
            case bit_16:
                return vm_store[2].label;
            case bit_8:
                return vm_store[3].label;
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
            if(!handler.tagged)
                continue;

            asmb::code_container_ptr container = handler.code;

            const reg_size reg_size = load_store_index_size(i);
            reg target_temp = get_bit_version(VTEMP, get_gpr_class_from_size(reg_size));

            if(reg_size == bit_32)
            {
                // we have to clear upper 32 bits of target register here
                container->add(encode(m_lea, ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, 8)));
                call_vm_handler(container, get_instruction_handler(m_pop, 1, reg_size));
                container->add(encode(m_mov, ZMEMBD(VTEMP2, 0, bit_64), ZIMMS(0)));
                container->add(encode(m_mov, ZMEMBD(VTEMP2, 0, reg_size), ZREG(target_temp)));
            }
            else
            {
                container->add(encode(m_lea, ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, 8)));
                call_vm_handler(container, get_instruction_handler(m_pop, 1, reg_size));
                container->add(encode(m_mov, ZMEMBD(VTEMP2, 0, reg_size), ZREG(target_temp)));
            }

            create_vm_return(container);

            context_stores.push_back(container);
        }

        return context_stores;
    }

    asmb::code_label_ptr inst_handlers::get_instruction_handler(mnemonic mnemonic, uint8_t operand_count, reg_size size)
    {
        std::tuple key = std::tie(mnemonic, operand_count, size);
        if(tagged_instruction_handlers.contains(key))
            return tagged_instruction_handlers[key];

        asmb::code_label_ptr label = asmb::code_label::create();
        tagged_instruction_handlers[key] = label;

        return label;
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_instruction_handlers()
    {
        std::vector<asmb::code_container_ptr> container;
        for(const auto& [key, label] : tagged_instruction_handlers)
        {
            auto [mnemonic, operand_count, size] = key;

            const std::shared_ptr<ir::handler::base_handler_gen> target_mnemonic = ir::instruction_handlers[mnemonic];
            ir::ir_insts handler_ir = target_mnemonic->gen_handler(get_gpr_class_from_size(size), operand_count);

            // todo: walk each block and guarantee that discrete_store variables only use vtemps we want
            ir::block_il_ptr ir_block = std::make_shared<ir::block_il>();
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
        if (vm_enter.tagged)
            handlers.push_back(build_vm_enter());

        if (vm_exit.tagged)
            handlers.push_back(build_vm_exit());

        if (vm_rflags_load.tagged)
            handlers.push_back(build_rflags_load());

        if (vm_rflags_save.tagged)
            handlers.push_back(build_rflags_save());

        handlers.append_range(build_context_load());
        handlers.append_range(build_context_store());

        handlers.append_range(build_instruction_handlers());

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
        code->add(encode(m_lea, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
        code->add(RECOMPILE(encode(m_mov, ZMEMBD(VCS, 0, 8), ZLABEL(return_label))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        code->add(RECOMPILE(encode(m_lea, ZREG(VIP), ZMEMBD(VBASE, target->get_address(), 8))));
        code->add(encode(m_jmp, ZREG(VIP)));

        // execution after VM handler should end up here
        code->bind(return_label);
    }

    void inst_handlers::create_vm_return(const asmb::code_container_ptr& container) const
    {
        // mov VCSRET, [VCS]        ; pop from call stack
        // lea VCS, [VCS + 8]       ; move up the call stack pointer
        container->add(encode(m_mov, ZREG(VCSRET), ZMEMBD(VCS, 0, 8)));
        container->add(encode(m_lea, ZREG(VCS), ZMEMBD(VCS, 8, 8)));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        container->add(encode(m_lea, ZREG(VIP), ZMEMBI(VBASE, VCSRET, 1, 8)));
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
