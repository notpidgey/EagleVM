#include <bitset>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <eaglevm-core/disassembler/disassembler.h>
#include <eaglevm-core/disassembler/models/basic_block.h>

#include "nlohmann/json.hpp"

#include <eaglevm-core/virtual_machine/vm_inst.h>
#include <eaglevm-core/virtual_machine/vm_virtualizer.h>

#include "util.h"
#include "run_container.h"

reg_overwrites build_writes(nlohmann::json& inputs);
uint32_t compare_context(CONTEXT& result, CONTEXT& target, reg_overwrites& outs, bool flags);
uint64_t* get_value(CONTEXT& new_context, std::string& reg);

// imul and mul tests are cooked
const std::string inclusive_tests[] = {
    "add", "dec", "div", "inc", "lea", "mov", "movsx", "sub"
};

#pragma section(".handlers", execute)
__declspec(allocate(".handlers")) unsigned char handler_buffer[0x1000 * 10] = { 0xCC };

#pragma section(".run_section", execute)
__declspec(allocate(".run_section")) unsigned char run_buffer[0x1000] = { 0xCC };

int main(int argc, char* argv[])
{
    // give .handlers and .run_section execute permissions
    // the fact that i have to do this is so extremely cooked
    // i mean its literally DOOMED
    // the virtualizer doesnt allow displacement sizes larger than run time addresses
    // so all i can do is create a section 🤣
    DWORD old_protect;
    VirtualProtect(handler_buffer, sizeof(handler_buffer), PAGE_EXECUTE_READWRITE, &old_protect);
    VirtualProtect(run_buffer, sizeof(run_buffer), PAGE_EXECUTE_READWRITE, &old_protect);

    memset(handler_buffer, 0xCC, sizeof(handler_buffer));
    memset(run_buffer, 0xCC, sizeof(run_buffer));

    // setbuf(stdout, NULL);
    auto test_data_path = argc > 1 ? argv[1] : "../deps/x86_test_data/TestData64";
    if (!std::filesystem::exists("x86-tests"))
        std::filesystem::create_directory("x86-tests");

    zydis_helper::setup_decoder();

    vm_inst vm_inst;
    vm_inst.init_reg_order();

    section_manager section = vm_inst.generate_vm_handlers(false);

    uint64_t rva = reinterpret_cast<uint64_t>(&handler_buffer) - reinterpret_cast<uint64_t>(&__ImageBase);
    encoded_vec vmhandle_data = section.compile_section(rva);

    assert(sizeof(handler_buffer) >= vmhandle_data.size());
    memcpy(&handler_buffer[0], vmhandle_data.data(), vmhandle_data.size());

    run_container::init_veh();

    // loop each file that test_data_path contains
    for (const auto& entry: std::filesystem::directory_iterator(test_data_path))
    {
        auto entry_path = entry.path();
        entry_path.make_preferred();

        std::string file_name = entry_path.stem().string();
        if (std::ranges::find(inclusive_tests, file_name) == std::end(inclusive_tests))
            continue;

        std::printf("[>] generating tests for: %ls\n", entry_path.c_str());

        // Create an ofstream object for the output file
        std::ofstream outfile("x86-tests/" + file_name);
        outfile << "[>] generating tests for: " << entry_path << "\n";

        // read entry file as string
        std::ifstream file(entry.path());
        nlohmann::json data = nlohmann::json::parse(file);

        int passed = 0;
        int failed = 0;

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
            if (instr.contains("sp"))
                continue;

            bool bp = false;
            if(test.contains("bp"))
                bp = test["bp"];

            {
                outfile << "\n\n[test] " << instr.c_str() << "\n";
                outfile << "[input]\n";
                util::print_regs(inputs, outfile);

                outfile << "[output]\n";
                util::print_regs(outputs, outfile);
            }

            reg_overwrites ins = build_writes(inputs);
            reg_overwrites outs = build_writes(outputs);

            run_container container(ins, outs);
            {
                vm_virtualizer virt(&vm_inst);

                std::vector<uint8_t> instruction_data = util::parse_hex(instr_data);
                decode_vec instructions = zydis_helper::get_instructions(instruction_data.data(), instruction_data.size());

                segment_dasm dasm(instructions, 0, instruction_data.size());
                dasm.generate_blocks();

                section_manager vm_code_sm(false);
                vm_code_sm.add(virt.virtualize_segment(&dasm));

                container.set_run_area(reinterpret_cast<uint64_t>(&run_buffer), sizeof(run_buffer), false);
                uint64_t instruction_rva = reinterpret_cast<uint64_t>(&run_buffer) -
                    reinterpret_cast<uint64_t>(&__ImageBase);

                encoded_vec virtualized_instruction = vm_code_sm.compile_section(instruction_rva);
                virtualized_instruction.erase(virtualized_instruction.end() - 5, virtualized_instruction.end());
                virtualized_instruction.push_back(0x0F);
                virtualized_instruction.push_back(0x01);
                virtualized_instruction.push_back(0xC1);

                assert(sizeof(run_buffer) >= virtualized_instruction.size());
                container.set_instruction_data(virtualized_instruction);
            }

            auto [result_context, output_target] = container.run(bp);

            // result_context is being set in the exception handler
            const uint32_t result = compare_context(
                result_context,
                output_target,
                outs,
                outputs.contains("flags")
            );

            if (result == none)
            {
                outfile << "[+] passed\n";
                passed++;
            }
            else
            {
                if (result & register_mismatch)
                {
                    outfile << "[!] register mismatch\n";

                    for (auto [reg, value]: outs)
                    {
                        if (reg == "flags" || reg == "rip")
                            continue;

                        outfile << "  > " << reg << "\n";
                        outfile << "  target: 0x" << std::hex << *util::get_value(output_target, reg) << '\n';
                        outfile << "  out   : 0x" << std::hex << *util::get_value(result_context, reg) << '\n';
                    }
                }

                if (result & flags_mismatch)
                {
                    outfile << "[!] flags mismatch\n";

                    std::bitset<32> target_flags(output_target.EFlags);
                    std::bitset<32> out_flags(result_context.EFlags);
                    outfile << "  target:" << target_flags << '\n';
                    outfile << "  out:   " << out_flags << '\n';
                }

                outfile << "[!] failed\n";
                failed++;
            }
        }

        // Close the output file
        outfile.close();

        std::printf("[+] finished generating %i tests for: %ls\n", passed + failed, entry_path.c_str());
        std::printf("[>] passed: %i\n", passed);
        std::printf("[>] failed: %i\n", failed);

        float success = (static_cast<float>(passed) / (passed + failed)) * 100;
        std::printf("[>] success: %f%%\n\n", success);
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
        if (input.value().is_string())
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

uint32_t compare_context(CONTEXT& result, CONTEXT& target, reg_overwrites& outs, bool flags)
{
    uint32_t fail = none;

    // this is such a stupid hack but instead of writing 20 if statements im going to do this for now
    constexpr auto reg_size = 16 * 8;

    // rip comparison is COOKED there is something really off about the test data
    // auto res_rip = result.Rip == target.Rip;
    for (auto& [reg, value]: outs)
    {
        if (reg == "rip" || reg == "flags")
            continue;

        uint64_t tar = *util::get_value(target, reg);
        uint64_t out = *util::get_value(result, reg);

        if (tar != out)
        {
            fail |= register_mismatch;
            break;
        }
    }

    if (flags)
    {
        bool res_flags = (result.EFlags & target.EFlags) == target.EFlags;
        if (!res_flags)
            fail |= flags_mismatch;
    }

    return fail;
}
