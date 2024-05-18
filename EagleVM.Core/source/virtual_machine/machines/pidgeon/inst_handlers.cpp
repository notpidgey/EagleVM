#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"

#include "eaglevm-core/util/random.h"

namespace eagle::virt::pidg
{
    inst_handlers::inst_handlers(const vm_inst_regs_ptr& push_order)
    {
        inst_regs = push_order;
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
    }

    asmb::code_label_ptr inst_handlers::get_context_load(codec::reg_size size)
    {
        switch(size)
        {
            case codec::bit_64:
                break;
            case codec::bit_32:
                break;
            case codec::bit_16:
                break;
            case codec::bit_8:
                break;
        }
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_context_load()
    {
    }

    asmb::code_label_ptr inst_handlers::get_context_store(codec::reg_size size)
    {
        switch(size)
        {
            case codec::bit_64:
                break;
            case codec::bit_32:
                break;
            case codec::bit_16:
                break;
            case codec::bit_8:
                break;
        }
    }

    std::vector<asmb::code_container_ptr> inst_handlers::build_context_store()
    {
    }

    asmb::code_label_ptr inst_handlers::get_instruction_handler(codec::mnemonic mnemonic, uint8_t operand_count, codec::reg_size size)
    {
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

        return handlers;
    }
}
