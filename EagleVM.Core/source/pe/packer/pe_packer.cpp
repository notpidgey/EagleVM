#include "eaglevm-core/pe/packer/pe_packer.h"
#include "eaglevm-core/pe/models/code_view_pdb.h"

#include "eaglevm-core/util/zydis_helper.h"
#include "eaglevm-core/util/section/function_container.h"
#include "eaglevm-core/util/section/section_manager.h"

void pe_packer::set_overlay(bool overlay)
{
    text_overlay = overlay;
}

std::pair<uint32_t, uint32_t> pe_packer::insert_pdb(encoded_vec& encoded_vec)
{
    uint32_t address = encoded_vec.size();

    code_view_pdb pdb{};
    pdb.signature[0] = 'R';
    pdb.signature[1] = 'S';
    pdb.signature[2] = 'D';
    pdb.signature[3] = 'S';
    pdb.age = 0xE;

    // inser the "pdb" structure to the back of encoded_vec
    encoded_vec.insert(encoded_vec.end(), reinterpret_cast<uint8_t*>(&pdb), reinterpret_cast<uint8_t*>(&pdb) + sizeof(pdb));

    std::string pdb_value;
    //pdb_value += "\n⠀⠀⠀⢀⡴⠋⠉⢉⠍⣉⡉⠉⠉⠉⠓⠲⠶⠤⣄\n";
    pdb_value += "⠀⠀⢀⠎⠀⠪⠾⢊⣁⣀⡀⠄⠀⠀⡌⠉⠁⠄⢳\n";
    pdb_value += "⠀⣰⠟⣢⣤⣐⠘⠛⣻⠻⠭⠇⠀⢤⡶⠟⠛⠂⠀⢌⢷\n";
    pdb_value += "⢸⢈⢸⠠⡶⠬⣉⡉⠁⠀⣠⢄⡀⠀⠳⣄⠑⠚⣏⠁⣪⠇\n";
    pdb_value += "⠀⢯⡊⠀⠹⡦⣼⣍⠛⢲⠯⢭⣁⣲⣚⣁⣬⢾⢿⠈⡜\n";
    pdb_value += "⠀⠀⠙⡄⠀⠘⢾⡉⠙⡟⠶⢶⣿⣶⣿⣶⣿⣾⣿⠀⡇\n";
    pdb_value += "⠀⠀⠀⠙⢦⣤⡠⡙⠲⠧⠀⣠⣇⣨⣏⣽⡹⠽⠏⠀⡇\n";
    pdb_value += "⠀⠀⠀⠀⠀⠈⠙⠦⢕⡋⠶⠄⣤⠤⠤⠤⠤⠂⡠⠀⡇\n";
    pdb_value += "⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠑⠒⠦⠤⣄⣀⣀⣀⣠⠔⠁\n";

    // conver the string to bytes and insert at the end of the file
    encoded_vec.insert(encoded_vec.end(), pdb_value.begin(), pdb_value.end());
    return {address, pdb_value.size() + sizeof(pdb)};
}

section_manager pe_packer::create_section()
{
    section_manager section_manager;

    // apply text overlay
    if (text_overlay)
    {
        function_container container;

        std::vector<zydis_encoder_request> target_instructions;
        int current_byte = 0;

        // if you want to use this feature add some kind of text file where the run directory for eaglevm is
        // ill keep this feature disabled by default
        std::ifstream file("1984.txt");
        std::stringstream buffer;
        buffer << file.rdbuf();

        std::string text = buffer.str();

        std::ranges::sort(generator->sections, [](auto& a, auto& b)
        {
            auto a_section = std::get<0>(a);
            auto b_section = std::get<0>(b);

            return a_section.PointerToRawData < b_section.PointerToRawData;
        });

        for (auto& [header, data] : generator->sections)
        {
            const std::string section_name = pe_generator::section_name(header);
            if (section_name != ".vmcode" && section_name != ".vmdata" && section_name != ".text")
                continue;

            header.Characteristics |= IMAGE_SCN_MEM_WRITE;
            const uint32_t section_rva = header.VirtualAddress;

            code_label* rel_label = code_label::create();
            container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(IP_RIP, -rel_label->get(), 8))));
            container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(GR_RAX, section_rva, 8))));

            for (int i = 0; i < data.size(); i += 4)
            {
                if (current_byte + 4 > text.size())
                    break;

                uint32_t target_value = *reinterpret_cast<uint32_t*>(&text[current_byte]);
                uint32_t current_value = *reinterpret_cast<uint32_t*>(&data[i]);

                // first we write the target
                *reinterpret_cast<uint32_t*>(&data[i]) = target_value;

                // then we find a way to get it back
                int32_t diff = target_value - current_value;
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_SUB, ZMEMBD(GR_RAX, 0, 4), ZIMMS(diff)));
                container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZREG(GR_RAX), ZIMMS(4)));

                current_byte += 4;
            }
        }

        section_manager.add(container);
    }

    // return to main
    {
        function_container container;
        auto orig_entry = generator->nt_headers.OptionalHeader.AddressOfEntryPoint;

        code_label* rel_label = code_label::create();
        container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(IP_RIP, -rel_label->get(), 8))));
        container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RAX), ZMEMBD(GR_RAX, orig_entry, 8))));
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(GR_RAX)));

        section_manager.add(container);
    }


    return section_manager;
}
