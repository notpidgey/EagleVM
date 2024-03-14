#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <vector>

typedef std::vector<std::pair<std::string, uint64_t>> reg_overwrites;
typedef std::pair<uint64_t, uint16_t> memory_range;

struct memory_range_hash {
    std::size_t operator()(const memory_range& mr) const {
        return std::hash<uint64_t>()(mr.first) ^ std::hash<uint16_t>()(mr.second);
    }
};

class run_container
{
public:
    explicit run_container(const std::vector<uint8_t>& data,
        const reg_overwrites& input, const reg_overwrites& output)
    {
        instructions = data;
        input_writes = input;
        output_writes = output;
    }

    std::pair<CONTEXT, CONTEXT> run();

    void set_result(PCONTEXT result);
    CONTEXT get_safe_context();

    static void init_veh();
    static void destroy_veh();

private:
    std::vector<uint8_t> instructions;
    CONTEXT result_context;
    CONTEXT safe_context;

    reg_overwrites input_writes;
    reg_overwrites output_writes;

    static CONTEXT build_context(const CONTEXT& safe, reg_overwrites& writes);

    inline static PVOID veh_handle;
    inline static std::mutex run_tests_mutex;
    inline static std::unordered_map<memory_range, run_container*, memory_range_hash> run_tests;

    static LONG CALLBACK veh_handler(EXCEPTION_POINTERS* info);
};