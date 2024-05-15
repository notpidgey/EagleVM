#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"

#include "eaglevm-core/util/random.h"

namespace eagle::virt::pidg
{
    inst_handlers::inst_handlers(const vm_inst_regs_ptr& push_order)
    {
        inst_regs = push_order;

        vm_enter = { asmb::code_container::create("vm_enter"), 0 };
        vm_exit = { asmb::code_container::create("vm_exit"), 0 };

        vm_rflags_load = { asmb::code_container::create("vm_rflags_load"), 0 };
        vm_rflags_save = { asmb::code_container::create("vm_rflags_save"), 0 };

        vm_load = { };
        vm_store = { };
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

    asmb::code_container_ptr inst_handlers::get_vm_enter(const bool reference)
    {
        if (reference)
            vm_enter.reference_count++;

        return vm_enter.code;
    }

    uint32_t inst_handlers::get_vm_enter_reference() const
    {
        return vm_enter.reference_count;
    }

    asmb::code_container_ptr inst_handlers::build_vm_enter()
    {
    }

    asmb::code_container_ptr inst_handlers::get_vm_exit(const bool reference)
    {
        if (reference)
            vm_exit.reference_count++;

        return vm_exit.code;
    }

    uint32_t inst_handlers::get_vm_exit_reference() const
    {
        return vm_exit.reference_count;
    }

    asmb::code_container_ptr inst_handlers::build_vm_exit()
    {
    }

    asmb::code_container_ptr inst_handlers::get_rlfags_load(const bool reference)
    {
        if (reference)
            vm_rflags_load.reference_count++;

        return vm_rflags_load.code;
    }

    uint32_t inst_handlers::get_rflags_load_reference() const
    {
        return vm_rflags_load.reference_count;
    }

    asmb::code_container_ptr inst_handlers::build_rflags_load()
    {
    }

    asmb::code_container_ptr inst_handlers::get_rflags_store(const bool reference)
    {
        if (reference)
            vm_rflags_save.reference_count++;

        return vm_rflags_save.code;
    }

    uint32_t inst_handlers::get_rflags_save_reference() const
    {
        return vm_rflags_save.reference_count;
    }

    asmb::code_container_ptr inst_handlers::build_rflags_save()
    {
    }

    std::vector<asmb::code_container_ptr> inst_handlers::get_handlers()
    {
        std::vector<asmb::code_container_ptr> handlers;
        if (get_vm_enter_reference())
            handlers.push_back(build_vm_enter());

        if (get_vm_exit_reference())
            handlers.push_back(build_vm_exit());

        if (get_rflags_load_reference())
            handlers.push_back(build_rflags_load());

        if (get_rflags_save_reference())
            handlers.push_back(build_rflags_save());

        return handlers;
    }
}
