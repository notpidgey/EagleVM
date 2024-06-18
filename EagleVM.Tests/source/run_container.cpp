#include "run_container.h"

#include <intrin.h>
#include <iostream>
#include "util.h"

#pragma optimize("", off)
std::pair<CONTEXT, CONTEXT> run_container::run(const bool bp)
{
    CONTEXT output_target, input_target;

    bool test_ran = false;
    RtlCaptureContext(&safe_context);

    add_veh();
    if (!test_ran)
    {
        test_ran = true;

        // clear registers that will receive writes
        input_target = clear_context(safe_context, output_writes);
        input_target = build_context(input_target, input_writes);

        output_target = build_context(safe_context, output_writes);

        // exception handler will redirect to this RIP
        int64_t rip_diff = output_target.Rip - input_target.Rip;
        input_target.Rip = reinterpret_cast<uint64_t>(run_area);
        output_target.Rip = input_target.Rip + rip_diff;

        int64_t rsp_diff = output_target.Rsp - input_target.Rsp;
        input_target.Rsp = safe_context.Rsp;
        output_target.Rsp = input_target.Rsp + rsp_diff;

        if (bp)
        {
            // so i can tell if its intentional in a debugger
            __nop();
            __debugbreak();
            __nop();
        }

        RtlRestoreContext(&input_target, nullptr);
    }

    remove_veh();
    return { result_context, output_target };
}
#pragma optimize("", on)

void run_container::set_result_context(const PCONTEXT result)
{
    result_context = *result;
}

CONTEXT run_container::get_safe_context()
{
    return safe_context;
}

void run_container::set_run_area(uint64_t address, uint32_t size)
{
    run_area = reinterpret_cast<void*>(address);
    run_area_size = size;
}

memory_range run_container::get_range()
{
    return {
        reinterpret_cast<uint64_t>(run_area),
        run_area_size
    };
}

void run_container::add_veh()
{
    std::lock_guard lock(run_tests_mutex);
    run_tests[this] = get_range();
}

void run_container::remove_veh()
{
    std::lock_guard lock(run_tests_mutex);
    run_tests.erase(this);
}

void* run_container::get_run_area() const
{
    return run_area;
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
    for (auto& [reg, value] : writes)
        *test_util::get_value(new_context, reg) = value;

    return new_context;
}

CONTEXT run_container::clear_context(const CONTEXT& safe, reg_overwrites& writes)
{
    CONTEXT new_context = safe;
    for (auto& [reg, value] : writes)
        *test_util::get_value(new_context, reg) = 0;

    return new_context;
}

LONG run_container::veh_handler(EXCEPTION_POINTERS* info)
{
    const uint64_t current_rip = info->ContextRecord->Rip;
    std::lock_guard lock(run_tests_mutex);

    bool found = false;
    for (auto& [key, ranges] : run_tests)
    {
        auto [low, size] = ranges;
        if (low <= current_rip && current_rip <= low + size)
        {
            found = true;

            key->set_result_context(info->ContextRecord);
            *info->ContextRecord = key->get_safe_context();
            break;
        }
    }

    if (found)
        return EXCEPTION_CONTINUE_EXECUTION;

    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        info->ContextRecord->Rip += 1;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
