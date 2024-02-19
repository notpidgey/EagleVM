#include "pe/packer/pe_packer.h"
#include "util/zydis_helper.h"
#include "util/section/function_container.h"
#include "util/section/section_manager.h"

void pe_packer::set_overlay(bool overlay)
{
    text_overlay = overlay;
}

section_manager pe_packer::create_section()
{
    section_manager section_manager;

    // the entry point of the PE needs to be changed

    // apply text overlay
    if(text_overlay)
    {
        function_container container;

        std::vector<zydis_encoder_request> target_instructions;
        int current_byte = 0;

        // if you want to use this feature add some kind of text file where the run directory for eaglevm is
        // ill keep this feature disabled by default
        std::ifstream file("intel.txt");
        std::stringstream buffer;
        buffer << file.rdbuf();

        std::string text = buffer.str();

        std::ranges::sort(generator->sections, [](auto& a, auto& b)
        {
            auto a_section = std::get<0>(a);
            auto b_section = std::get<0>(b);

            return a_section.PointerToRawData < b_section.PointerToRawData;
        });

        for(auto& [header, data] : generator->sections)
        {
            const std::string section_name = pe_generator::section_name(header);
            if(section_name != ".vmcode" && section_name != ".vmdata")
                continue;

            const uint32_t section_rva = header.VirtualAddress;

            code_label* rel_label = code_label::create();
            container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
            container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(GR_RAX, section_rva, 8))));

            for(int i = 0; i < data.size(); i += 4)
            {
                if(current_byte + 4 > text.size())
                    break;

                uint32_t target_value = *reinterpret_cast<uint32_t*>(&text[current_byte]);
                uint32_t current_value = *reinterpret_cast<uint32_t*>(&data[i]);

                // first we write the target
                *reinterpret_cast<uint32_t*>(&data[i]) = target_value;

                // then we find a way to get it back
                int32_t diff = target_value - current_value;
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_SUB, ZMEMBD(GR_RAX, 0, 4), ZIMMS(diff)));
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZREG(GR_RAX), ZIMMS(4)));

                current_byte += 8;
            }
        }

        section_manager.add(container);
    }

    // return to main
    {
        function_container container;

        code_label* rel_label = code_label::create();
        container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(IP_RIP, -rel_label->get() - 7, 8))));
        container.add(RECOMPILE(
                zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(GR_RAX, generator->nt_headers.OptionalHeader.AddressOfEntryPoint, 8))));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(GR_RAX)));

        section_manager.add(container);
    }


    return section_manager;
}