#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

#include "nlohmann/json.hpp"

#include "run.h"

CONTEXT run_context, exit_context;
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
        std::printf("[>] exception occured, reverting context");
        result_context = *info->ContextRecord;
        *info->ContextRecord = safe_context;
    }
    else if (info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        std::printf("[>] redirecting to shellcode");
        info->ContextRecord->Rip = run_context.Rip;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

CONTEXT build_context(nlohmann::json& inputs, CONTEXT& safe_context)
{
    CONTEXT input_context = safe_context;
    for (auto& input: inputs.items())
    {
        std::string reg = input.key();
        std::string val = input.value();

        switch (reg)
        {
            case "rax":
                input_context.Rax = std::stoull(val, nullptr, 16);
                break;
            case "rcx":
                input_context.Rcx = std::stoull(val, nullptr, 16);
                break;
            case "rdx":
                input_context.Rdx = std::stoull(val, nullptr, 16);
                break;
            case "rbx":
                input_context.Rbx = std::stoull(val, nullptr, 16);
                break;
            case "rsi":
                input_context.Rsi = std::stoull(val, nullptr, 16);
                break;
            case "rdi":
                input_context.Rdi = std::stoull(val, nullptr, 16);
                break;
            case "rsp":
                input_context.Rsp = std::stoull(val, nullptr, 16);
                break;
            case "rbp":
                input_context.Rbp = std::stoull(val, nullptr, 16);
                break;
            case "r8":
                input_context.R8 = std::stoull(val, nullptr, 16);
                break;
            case "r10":
                input_context.R10 = std::stoull(val, nullptr, 16);
                break;
            case "r11":
                input_context.R11 = std::stoull(val, nullptr, 16);
                break;
            case "r12":
                input_context.R12 = std::stoull(val, nullptr, 16);
                break;
            case "r13":
                input_context.R13 = std::stoull(val, nullptr, 16);
                break;
            case "r14":
                input_context.R14 = std::stoull(val, nullptr, 16);
                break;
            case "r15":
                input_context.R15 = std::stoull(val, nullptr, 16);
                break;
            case "flags":
                input_context.EFlags = std::stoull(val, nullptr, 16);
        }
    }

    return input_context;
}

bool compare_context(CONTEXT& result, CONTEXT& target)
{
    // this is such a stupid hack but instead of writing 20 if statements im going to do this for now
    constexpr auto reg_size = 16 * 8;
    auto res = memcmp(&result.Rax, &target.Rax, reg_size);

    // TODO: add RIP check but we dont really care about that
    return res == 0;
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

                run_context = build_context(inputs, safe_context);
                exit_context = build_context(outputs, safe_context);

                // exception handler will redirect to this RIP
                run_context.Rip = reinterpret_cast<uint64_t>(instruction_memory);

                // we should get an exception which will return us back to before the if statement
                // this exception will happen because after RIP is advanced, its out of executable memory
                run_shellcode(&instruction_data[0], &run_context);
            }

            if (!compare_context(result_context, exit_context))
                std::printf("[-] test failed: %s\n", instr.c_str());
            else
                std::printf("[+] test passed: %s\n", instr.c_str());

            free(instruction_memory);
        }
    }
}
