#include "run_container.h"

#include <intrin.h>

#include "util.h"

#pragma optimize("", off)
std::pair<CONTEXT, CONTEXT> run_container::run()
{
    if(!run_area)
        create_run_area();

    memcpy(run_area, instructions.data(), instructions.size());
    CONTEXT output_target, input_target;

    bool test_ran = false;
    RtlCaptureContext(&safe_context);

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

        RtlRestoreContext(&input_target, nullptr);
    }

    if(clear_run_area)
        free_run_area();
    else
        memset(run_area, 0xCC, run_area_size);

    return {result_context, output_target};
}
#pragma optimize("", on)

void run_container::set_instruction_data(const std::vector<uint8_t>& data)
{
    instructions = data;
}

void run_container::set_result(const PCONTEXT result)
{
    result_context = *result;
}

CONTEXT run_container::get_safe_context()
{
    return safe_context;
}

memory_range run_container::create_run_area(const uint32_t size)
{
    if(run_area)
        free_run_area();

    run_area_size = size;
    run_area = VirtualAlloc(
        nullptr,
        size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    memset(run_area, 0xCC, run_area_size);

    const memory_range mem_range = {
        reinterpret_cast<uint64_t>(run_area),
        run_area_size
    };
    {
        std::lock_guard lock(run_tests_mutex);
        run_tests[mem_range] = this;
    }

    return mem_range;
}

void run_container::set_run_area(uint64_t address, uint32_t size, bool clear)
{
    run_area = reinterpret_cast<void*>(address);
    run_area_size = size;
    clear_run_area = clear;

    const memory_range mem_range = {
        reinterpret_cast<uint64_t>(run_area),
        run_area_size
    };
    {
        std::lock_guard lock(run_tests_mutex);
        run_tests[mem_range] = this;
    }
}

void run_container::free_run_area()
{
    const memory_range mem_range = {
        reinterpret_cast<uint64_t>(run_area),
        run_area_size
    };

    {
        std::lock_guard lock(run_tests_mutex);

        VirtualFree(run_area, 0x1000, MEM_RELEASE);
        run_tests.erase(mem_range);
    }

    run_area = nullptr;
    run_area_size = 0;
}

void* run_container::get_run_area()
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
    for (auto& [reg, value]: writes)
        *util::get_value(new_context, reg) = value;

    return new_context;
}

CONTEXT run_container::clear_context(const CONTEXT& safe, reg_overwrites& writes)
{
    CONTEXT new_context = safe;
    for (auto& [reg, value]: writes)
        *util::get_value(new_context, reg) = 0;

    return new_context;
}

LONG run_container::veh_handler(EXCEPTION_POINTERS* info)
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

    return EXCEPTION_CONTINUE_SEARCH;
}
