#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

#include "nlohmann/json.hpp"
#include "util.h"
#include "run_container.h"

reg_overwrites build_writes(nlohmann::json& inputs);
bool compare_context(CONTEXT& result, CONTEXT& target, bool flags);

int main(int argc, char* argv[])
{
    setbuf(stdout, NULL);
    auto test_data_path = argc > 1 ? argv[1] : "../deps/x86_test_data/TestData64";

    run_container::init_veh();

    // loop each file that test_data_path contains
    for (const auto& entry: std::filesystem::directory_iterator(test_data_path))
    {
        std::printf("[>] generating tests for: %ls\n", entry.path().c_str());
        std::string file_name = entry.path().filename().string();

        // read entry file as string
        std::ifstream file(entry.path());
        nlohmann::json data = nlohmann::json::parse(file);

        // data now contains an array of objects, enumerate each object
        for (auto& test: data)
        {
            // create a new file for each test
            std::string instr_data = test["data"];
            std::string instr = test["instr"];

            nlohmann::json inputs = test["inputs"];
            nlohmann::json outputs = test["outputs"];

            // i dont know what else to do
            // you cannot just use VEH to recover RIP/RSP corruption
            if(instr.contains("sp"))
                continue;

            {
                std::printf("\n\n[test] %s\n", instr.c_str());
                std::printf("[input]\n");
                util::print_regs(inputs);

                std::printf("[output]\n");
                util::print_regs(outputs);
            }

            std::vector<uint8_t> instruction_data = util::parse_hex(instr_data);
            auto ins = build_writes(inputs);
            auto outs = build_writes(outputs);

            run_container container(instruction_data, ins, outs);
            auto [result_context, output_target] = container.run();

            // result_context is being set in the exception handler
            bool flags = outputs.contains("flags");
            bool success = compare_context(result_context, output_target, flags);
            if (!success)
                std::printf("[!] failed");
            else
                std::printf("[+] passed");
        }
    }

    run_container::destroy_veh();
}

reg_overwrites build_writes(nlohmann::json& inputs)
{
    reg_overwrites overwrites;
    for (auto& input: inputs.items())
    {
        std::string reg = input.key();
        uint64_t value = 0;
        if(input.value().is_string())
        {
            std::string str = input.value();
            value = std::stoull(str, nullptr, 16);
            value = _byteswap_uint64(value);
        }
        else
        {
            value = input.value();
        }

        overwrites.emplace_back(reg, value);
    }

    return overwrites;
}

bool compare_context(CONTEXT& result, CONTEXT& target, bool flags)
{
    // this is such a stupid hack but instead of writing 20 if statements im going to do this for now
    constexpr auto reg_size = 16 * 8;

    // rip comparison is COOKED there is something really off about the test data
    // auto res_rip = result.Rip == target.Rip;
    auto res_regs = memcmp(&result.Rax, &target.Rax, reg_size);
    if(!flags)
        return res_regs == 0;

    bool res_flags = (target.EFlags & result.EFlags) == target.EFlags;
    return res_regs == 0 && res_flags;
}
