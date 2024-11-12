#include "eaglevm-core/pe/packer/pe_packer.h"

#include <sstream>

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/pe/models/code_view_pdb.h"

#include "eaglevm-core/compiler/section_manager.h"

namespace eagle::pe
{
    void pe_packer::set_overlay(bool overlay)
    {
        text_overlay = overlay;
    }

    std::pair<uint32_t, uint32_t> pe_packer::insert_pdb(codec::encoded_vec& encoded_vec)
    {
        size_t address = encoded_vec.size();

        code_view_pdb pdb{ };
        pdb.signature[0] = 'R';
        pdb.signature[1] = 'S';
        pdb.signature[2] = 'D';
        pdb.signature[3] = 'S';
        pdb.age = 0xE;

        // insert the "pdb" structure to the back of encoded_vec
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

        // convert the string to bytes and insert at the end of the file
        encoded_vec.insert(encoded_vec.end(), pdb_value.begin(), pdb_value.end());
        return { address, pdb_value.size() + sizeof(pdb) };
    }

    asmb::section_manager pe_packer::create_section() const
    {
        using namespace codec;

        asmb::section_manager section_manager;

        // apply text overlay
        if (text_overlay)
        {
            encoder::encode_builder builder = { };

            std::vector<enc::req> target_instructions;
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

                return a_section.ptr_raw_data < b_section.ptr_raw_data;
            });

            for (auto& [header, data] : generator->sections)
            {
                const auto section_name = header.name.to_string();
                if (section_name != ".vmcode" && section_name != ".vmdata" && section_name != ".text")
                    continue;

                header.characteristics.mem_write = 1;
                const uint32_t section_rva = header.virtual_address;

                asmb::code_label_ptr rel_label = asmb::code_label::create();
                builder.label(rel_label)
                       .make(m_lea, encoder::reg_op(rax), encoder::mem_op(rip, -rel_label->get_address(), 8))
                       .make(m_lea, encoder::reg_op(rax), encoder::mem_op(rax, section_rva, 8));

                for (int i = 0; i < data.size(); i += 4)
                {
                    if (current_byte + 4 > text.size())
                        break;

                    uint32_t target_value = *reinterpret_cast<uint32_t*>(&text[current_byte]);
                    uint32_t current_value = *reinterpret_cast<uint32_t*>(&data[i]);

                    // first we write the target
                    *reinterpret_cast<uint32_t*>(&data[i]) = target_value;

                    // then we find a way to get it back
                    const int32_t diff = target_value - current_value;
                    builder.make(m_sub, encoder::mem_op(rax, 0, 4), encoder::imm_op(diff))
                           .make(m_add, encoder::reg_op(rax), encoder::imm_op(4));

                    current_byte += 4;
                }
            }

            asmb::code_container_ptr container = asmb::code_container::create();
            container->transfer_from(builder);

            section_manager.add_code_container(container);
        }

        // return to main
        {
            encoder::encode_builder builder = { };
            auto orig_entry = generator->nt_headers.OptionalHeader.AddressOfEntryPoint;

            asmb::code_label_ptr rel_label = asmb::code_label::create();
            builder.label(rel_label)
                   .make(m_lea, encoder::reg_op(rax), encoder::mem_op(rip, -rel_label->get_address(), 8))
                   .make(m_lea, encoder::reg_op(rax), encoder::mem_op(rax, orig_entry, 8))
                   .make(m_jmp, encoder::reg_op(rax));

            asmb::code_container_ptr container = asmb::code_container::create();
            container->transfer_from(builder);

            section_manager.add_code_container(container);
        }

        return section_manager;
    }
}
