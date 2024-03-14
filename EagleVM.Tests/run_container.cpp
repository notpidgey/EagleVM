#include "run_container.h"

#include "util.h"

#pragma optimize("", off)
std::pair<CONTEXT, CONTEXT> run_container::run()
{
    void* instruction_memory = VirtualAlloc(
        nullptr,
        0x1000,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    memset(instruction_memory, 0xCC, 0x1000);
    memcpy(instruction_memory, instructions.data(), instructions.size());

    const memory_range mem_range = {
        reinterpret_cast<uint64_t>(instruction_memory),
        static_cast<uint16_t>(0x1000)
    };
    {
        std::lock_guard lock(run_tests_mutex);
        run_tests[mem_range] = this;
    }

    CONTEXT output_target, input_target;

    bool test_ran = false;
    RtlCaptureContext(&safe_context);

    if (!test_ran)
    {
        test_ran = true;

        input_target = build_context(safe_context, input_writes);
        output_target = build_context(safe_context, output_writes);

        // exception handler will redirect to this RIP
        int64_t rip_diff = output_target.Rip - input_target.Rip;
        input_target.Rip = reinterpret_cast<uint64_t>(instruction_memory);
        output_target.Rip = input_target.Rip + rip_diff;

        int64_t rsp_diff = output_target.Rsp - input_target.Rsp;
        input_target.Rsp = safe_context.Rsp;
        output_target.Rsp = input_target.Rsp + rsp_diff;

        RtlRestoreContext(&input_target, nullptr);
    }

    {
        std::lock_guard lock(run_tests_mutex);

        VirtualFree(instruction_memory, 0x1000, MEM_RELEASE);
        run_tests.erase(mem_range);
    }

    return {result_context, output_target};
}
#pragma optimize("", on)

void run_container::set_result(const PCONTEXT result)
{
    result_context = *result;
}

CONTEXT run_container::get_safe_context()
{
    return safe_context;
}

void run_container::init_veh()
{
    veh_handle = AddVectoredExceptionHandler(1, veh_handler);
}

void run_container::destroy_veh()
{
    RemoveVectoredExceptionHandler(veh_handler);
}

CONTEXT run_container::build_context(const CONTEXT& safe, reg_overwrites& writes)
{
    CONTEXT new_context = safe;
    for (auto& [reg, value]: writes)
        *util::get_value(new_context, reg) = value;

    return new_context;
}

LONG run_container::veh_handler(EXCEPTION_POINTERS* info)
{
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION ||
        info->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION ||
        info->ExceptionRecord->ExceptionCode == EXCEPTION_DEBUG_EVENT ||
        info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        uint64_t current_rip = info->ContextRecord->Rip;

        std::lock_guard lock(run_tests_mutex);

        bool found = false;
        for (auto& [ranges, val]: run_tests)
        {
            auto [low, size] = ranges;
            if (low <= current_rip && current_rip <= low + size)
            {
                found = true;

                val->set_result(info->ContextRecord);
                *info->ContextRecord = val->get_safe_context();
                break;
            }
        }

        if (found)
            return EXCEPTION_CONTINUE_EXECUTION;

        __debugbreak();
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
