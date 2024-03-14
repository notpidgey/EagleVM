#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

#include "nlohmann/json.hpp"

CONTEXT input_target, output_target;
CONTEXT safe_context, result_context;

std::vector<uint8_t> parse_hex(const std::string& hex)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        char byte = (char) strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

LONG CALLBACK shellcode_handler(EXCEPTION_POINTERS* info)
{
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        std::printf("[>] exception occured, reverting context\n");
        result_context = *info->ContextRecord;
        *info->ContextRecord = safe_context;

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

CONTEXT build_context(nlohmann::json& inputs, CONTEXT& safe_context)
{
    CONTEXT input_context = safe_context;
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
        
        if (reg == "rax") {
            input_context.Rax = value;
        } else if (reg == "rcx") {
            input_context.Rcx = value;
        } else if (reg == "rdx") {
            input_context.Rdx = value;
        } else if (reg == "rbx") {
            input_context.Rbx = value;
        } else if (reg == "rsi") {
            input_context.Rsi = value;
        } else if (reg == "rdi") {
            input_context.Rdi = value;
        } else if (reg == "rsp") {
            input_context.Rsp = value;
        } else if (reg == "rbp") {
            input_context.Rbp = value;
        } else if (reg == "r8") {
            input_context.R8 = value;
        } else if (reg == "r10") {
            input_context.R10 = value;
        } else if (reg == "r11") {
            input_context.R11 = value;
        } else if (reg == "r12") {
            input_context.R12 = value;
        } else if (reg == "r13") {
            input_context.R13 = value;
        } else if (reg == "r14") {
            input_context.R14 = value;
        } else if (reg == "r15") {
            input_context.R15 = value;
        } else if (reg == "flags") {
            input_context.EFlags = input.value();
        }
    }

    return input_context;
}

bool compare_context(CONTEXT& result, CONTEXT& target)
{
    // this is such a stupid hack but instead of writing 20 if statements im going to do this for now
    constexpr auto reg_size = 16 * 8;
    auto res_regs = memcmp(&result.Rax, &target.Rax, reg_size);
    bool res_flags = true; //target.EFlags & result.EFlags == target.EFlags;

    // TODO: add RIP check but we dont really care about that
    return res_regs == 0 && res_flags;
}

int main(int argc, char* argv[])
{
    auto test_data_path = argc > 1 ? argv[1] : "../deps/x86_test_data/TestData64";

    AddVectoredExceptionHandler(1, shellcode_handler);

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

            std::vector<uint8_t> instruction_data = parse_hex(instr_data);

            void* instruction_memory = VirtualAlloc(
                nullptr,
                instruction_data.size(),
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
            );

            if (instruction_memory == nullptr)
            {
                std::printf("[-] failed to allocate memory for shellcode\n");
                return 1;
            }

            memcpy(instruction_memory, &instruction_data[0], instruction_data.size());

            bool test_ran = false;

            RtlCaptureContext(&safe_context);
            if(!test_ran)
            {
                test_ran = true;

                input_target = build_context(inputs, safe_context);
                output_target = build_context(outputs, safe_context);

                // exception handler will redirect to this RIP
                input_target.Rip = reinterpret_cast<uint64_t>(instruction_memory);
                RtlRestoreContext(&input_target, nullptr);
            }

            // result_context is being set in the exception handler

            bool success = compare_context(result_context, output_target);
            if (!success)
                std::printf("[-] test failed: %s\n", instr.c_str());
            else
                std::printf("[+] test passed: %s\n", instr.c_str());

            VirtualFree(instruction_memory, instruction_data.size(), MEM_RELEASE);
        }
    }
}