#include "util.h"

#include <fstream>

std::vector<uint8_t> test_util::parse_hex(const std::string& hex)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        const char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}

void test_util::print_regs(nlohmann::json& inputs, std::stringstream& stream)
{
    for (auto& input: inputs.items())
    {
        const std::string& key = input.key();
        uint64_t value = 0;

        if(key == "flags")
        {
            value = input.value();
        }
        else
        {
            std::string str = input.value();
            value = std::stoull(str, nullptr, 16);
            value = _byteswap_uint64(value);
        }

        stream << "  " << key << " : 0x" << std::hex << value << "\n";
    }
}

uint64_t* test_util::get_value(CONTEXT& new_context, std::string& reg)
{
    if (reg == "rip")
        return &new_context.Rip ;
    else if (reg == "rax")
        return &new_context.Rax ;
    else if (reg == "rcx")
        return &new_context.Rcx ;
    else if (reg == "rdx")
        return &new_context.Rdx ;
    else if (reg == "rbx")
        return &new_context.Rbx ;
    else if (reg == "rsi")
        return &new_context.Rsi ;
    else if (reg == "rdi")
        return &new_context.Rdi ;
    else if (reg == "rsp")
        return &new_context.Rsp ;
    else if (reg == "rbp")
        return &new_context.Rbp ;
    else if (reg == "r8")
        return &new_context.R8 ;
    else if (reg == "r9")
        return &new_context.R9 ;
    else if (reg == "r10")
        return &new_context.R10 ;
    else if (reg == "r11")
        return &new_context.R11 ;
    else if (reg == "r12")
        return &new_context.R12 ;
    else if (reg == "r13")
        return &new_context.R13 ;
    else if (reg == "r14")
        return &new_context.R14 ;
    else if (reg == "r15")
        return &new_context.R15 ;
    else if (reg == "flags")
        return reinterpret_cast<uint64_t *>(&new_context.EFlags);
}

