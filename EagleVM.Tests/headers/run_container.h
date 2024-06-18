#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <vector>

typedef std::vector<std::pair<std::string, uint64_t>> reg_overwrites;
typedef std::pair<uint64_t, uint32_t> memory_range;

struct memory_range_hash {
    std::size_t operator()(const memory_range& mr) const {
        return std::hash<uint64_t>()(mr.first) ^ std::hash<uint16_t>()(mr.second);
    }
};

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

    void set_instruction_data(const std::vector<uint8_t>& data);
    void set_result(PCONTEXT result);
    CONTEXT get_safe_context();

    void set_run_area(uint64_t address, uint32_t size, bool clear);
    void* get_run_area();

    static void init_veh();
    static void destroy_veh();

private:
    void* run_area = nullptr;
    uint32_t run_area_size = 0;
    bool clear_run_area = true;

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
    inline static std::unordered_map<memory_range, run_container*, memory_range_hash> run_tests;

    static LONG CALLBACK veh_handler(EXCEPTION_POINTERS* info);
};