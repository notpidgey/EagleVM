#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <vector>

typedef std::vector<std::pair<std::string, uint64_t>> reg_overwrites;
typedef std::pair<uint64_t, uint32_t> memory_range;

class run_container
{
public:
    run_container(const std::vector<uint8_t>& data,
        const reg_overwrites& input, const reg_overwrites& output)
    {
        input_writes = input;
        output_writes = output;

        result_context = {};
        safe_context = {};
    }

    run_container(const reg_overwrites& input, const reg_overwrites& output)
    {
        input_writes = input;
        output_writes = output;

        result_context = {};
        safe_context = {};
    }

    std::pair<CONTEXT, CONTEXT> run(bool bp = false);

    void set_result_context(PCONTEXT result);
    CONTEXT get_safe_context();

    void set_run_area(uint64_t address, uint32_t size);
    void* get_run_area() const;

    static void init_veh();
    static void destroy_veh();

private:
    void* run_area = nullptr;
    uint32_t run_area_size = 0;

    CONTEXT result_context{};
    CONTEXT safe_context{};

    reg_overwrites input_writes;
    reg_overwrites output_writes;

    memory_range get_range();
    void add_veh();
    void remove_veh();

    static CONTEXT build_context(const CONTEXT& safe, reg_overwrites& writes);
    static CONTEXT clear_context(const CONTEXT& safe, reg_overwrites& writes);

    inline static PVOID veh_handle;
    inline static std::mutex run_tests_mutex;
    inline static std::unordered_map<run_container*, memory_range> run_tests;

    static LONG CALLBACK veh_handler(EXCEPTION_POINTERS* info);
};