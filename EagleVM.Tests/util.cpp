#include "util.h"

std::vector<uint8_t> util::parse_hex(const std::string& hex)
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

void util::print_regs(nlohmann::json& inputs)
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

        std::printf("  %s : 0x%lx\n", key.c_str(), value);
    }
}
