#include <algorithm>
#include <ranges>

#include "eaglevm-core/compiler/section_manager.h"
#include "eaglevm-core/pe/pe_parser.h"
#include "eaglevm-core/pe/pe_generator.h"
#include "eaglevm-core/pe/packer/pe_packer.h"

#include "eaglevm-core/disassembler/disassembler.h"
#include "eaglevm-core/virtual_machine/ir/ir_translator.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/machine.h"

using namespace eagle;

int main(int argc, char* argv[])
{
    auto executable = argc > 1 ? argv[1] : "EagleVMSandbox.exe";

    eagle::pe::pe_parser parser = eagle::pe::pe_parser(executable);
    if (!parser.read_file())
    {
        std::printf("[!] failed to read file: %s\n", executable);
        return EXIT_FAILURE;
    }

    std::printf("[+] loaded %s -> %i bytes\n", executable, parser.get_file_size());

    int i = 1;
    std::vector<PIMAGE_SECTION_HEADER> sections = parser.get_sections();

    std::printf("[>] image sections\n");
    std::printf("%3s %-10s %-10s %-10s\n", "", "name", "va", "size");
    std::ranges::for_each
    (
        sections.begin(), sections.end(),
        [&i](const PIMAGE_SECTION_HEADER image_section)
        {
            std::printf
            (
                "%3i %-10s %-10lu %-10lu\n", i, image_section->Name,
                image_section->VirtualAddress, image_section->SizeOfRawData);
            i++;
        });

    i = 1;

    std::printf("\n[>] image imports\n");
    std::printf("%3s %-20s %-s\n", "", "source", "import");
    parser.enum_imports(
        [&i](const PIMAGE_IMPORT_DESCRIPTOR import_descriptor, const PIMAGE_THUNK_DATA thunk_data,
        const PIMAGE_SECTION_HEADER import_section, int, const uint8_t* data_base)
        {
            const uint8_t* import_section_raw = data_base + import_section->PointerToRawData;
            const char* import_library = reinterpret_cast<const char*>(import_section_raw + (import_descriptor->Name -
                import_section->VirtualAddress));
            const char* import_name = reinterpret_cast<const char*>(import_section_raw + (thunk_data->u1.AddressOfData -
                import_section->VirtualAddress + 2));

            std::printf("%3i %-20s %-s\n", i, import_library, import_name);
            i++;
        });

    std::printf("\n[+] porting original pe properties to new executable...\n");

    auto nt_header = parser.get_nt_header();
    std::printf("[>] image base -> 0x%llx bytes\n", nt_header->OptionalHeader.ImageBase);
    std::printf("[>] section alignment -> %lu bytes\n", nt_header->OptionalHeader.SectionAlignment);
    std::printf("[>] file alignment -> %lu bytes\n", nt_header->OptionalHeader.FileAlignment);
    std::printf("[>] stack reserve -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfStackReserve);
    std::printf("[>] stack commit -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfStackCommit);
    std::printf("[>] heap reserve -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfHeapReserve);
    std::printf("[>] heap commit -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfHeapCommit);

    std::printf("\n[>] searching for uses of vm macros...\n");

    i = 1;
    std::vector<std::pair<uint32_t, pe::stub_import>> vm_iat_calls = parser.find_iat_calls();
    std::printf("%3s %-10s %-10s\n", "", "rva", "vm");
    std::ranges::for_each
    (
        vm_iat_calls.begin(), vm_iat_calls.end(),
        [&i, &parser](const auto call)
        {
            auto [file_offset, stub_import] = call;
            switch (stub_import)
            {
                case pe::stub_import::vm_begin:
                    std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "vm_begin");
                    break;
                case pe::stub_import::vm_end:
                    std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "vm_end");
                    break;
                case pe::stub_import::unknown:
                    std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "?");
                    break;
            }

            i++;
        });

    std::printf("\n");

    //vm macros should be in "begin, end, begin, end" order
    //this is not an entirely reliable way to check because it could mean function A has vm_begin and function B will have vm_end
    //meaning it will virtualize in between the two functions, which should never happen.
    //blame the user

    bool success = true;
    pe::stub_import previous_call = pe::stub_import::vm_end;
    std::ranges::for_each
    (
        vm_iat_calls.begin(), vm_iat_calls.end(),
        [&previous_call, &success](const auto call)
        {
            if (call.second == previous_call)
            {
                success = false;
                return false;
            }

            previous_call = call.second;
            return true;
        });

    if (!success)
    {
        std::printf("[+] failed to verify macro usage\n");
        exit(-1);
    }

    std::printf("[+] successfully verified macro usage\n");

    //to keep relative jumps of the image intact, it is best to just stick the vm section at the back of the pe
    pe::pe_generator generator(&parser);
    generator.load_parser();

    IMAGE_SECTION_HEADER* last_section = &std::get<0>(generator.get_last_section());

    // its not a great idea to split up the virtual machines into a different section than the virtualized code
    // as it will aid reverse engineers in understanding what is happening in the binary
    auto& [data_section, data_section_bytes] = generator.add_section(".haihallo");
    data_section.PointerToRawData = generator.align_file(last_section->PointerToRawData + last_section->SizeOfRawData);
    data_section.SizeOfRawData = 0;
    data_section.VirtualAddress = generator.align_section(last_section->VirtualAddress + last_section->Misc.VirtualSize);
    data_section.Misc.VirtualSize = 0;
    data_section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_CODE;
    data_section.PointerToRelocations = 0;
    data_section.NumberOfRelocations = 0;
    data_section.NumberOfLinenumbers = 0;

    last_section = &data_section;

    std::printf("[>] generating vm handlers at %04X...\n", static_cast<uint32_t>(data_section.VirtualAddress));
    std::printf("\n[>] generating virtualized code...\n\n");

    std::vector<std::pair<uint32_t, uint32_t>> va_nop;
    std::vector<std::pair<uint32_t, uint32_t>> va_ran;
    std::vector<std::pair<uint32_t, asmb::code_label_ptr>> va_enters;

    asmb::section_manager vm_section(true);
    for (int c = 0; c < vm_iat_calls.size(); c += 2) // i1 = vm_begin, i2 = vm_end
    {
        constexpr uint8_t call_size_64 = 6;

        uint32_t rva_inst_begin = parser.offset_to_rva(vm_iat_calls[c].first) + call_size_64;
        uint32_t rva_inst_end = parser.offset_to_rva(vm_iat_calls[c + 1].first);

        std::printf("[+] function %i-%i\n", c, c + 1);
        std::printf("\t[>] instruction begin: 0x%x\n", rva_inst_begin);
        std::printf("\t[>] instruction end: 0x%x\n", rva_inst_end);
        std::printf("\t[>] instruction size: %u\n", rva_inst_end - rva_inst_begin);

        uint8_t* pinst_begin = parser.rva_to_pointer(rva_inst_begin);
        uint8_t* pinst_end = parser.rva_to_pointer(rva_inst_end);

        /*
         * this approach is not a good idea, but its easy to solve
         * the problem is that this gives us a limited scope of the context
         * there should be some kind of whole-program context builder because then
         * we will be able to chain every virtualized code section together.
         * but this creates sort of a mess which i dont really like
         */

        codec::decode_vec instructions = codec::get_instructions(pinst_begin, pinst_end - pinst_begin);

        dasm::segment_dasm dasm(std::move(instructions), rva_inst_begin, rva_inst_end);
        dasm.generate_blocks();

        std::printf("\t[>] dasm found %llu basic blocks\n", dasm.blocks.size());

        ir::ir_translator ir_trans(&dasm);
        ir::ir_preopt_block_vec preopt = ir_trans.translate(true);

        // here we assign vms to each block
        // for the current example we can assign a unique vm to each block
        uint32_t vm_index = 0;
        std::vector<ir::ir_preopt_vm_id> block_vm_ids;
        for (const auto& preopt_block : preopt)
            block_vm_ids.emplace_back(preopt_block, vm_index++);

        // if we want, we can do a little optimzation which will rewrite the preopt blocks
        // or we could simply ir_trans.flatten()
        std::vector<ir::ir_block_vm_id> vm_blocks = ir_trans.optimize(block_vm_ids);

        // we want the same settings for every machine
        virt::pidg::settings_ptr settings = std::make_shared<virt::pidg::settings>();
        settings->set_temp_count(4);
        settings->set_randomize_vm_regs(true);
        settings->set_randomize_stack_regs(true);

        // initialize block code labels
        std::unordered_map<ir::block_il_ptr, asmb::code_label_ptr> block_labels;
        for (auto& blocks : vm_blocks | std::views::keys)
            for (const auto& block : blocks)
                block_labels[block] = asmb::code_label::create();

        asmb::code_label_ptr entry_point = nullptr;
        for (const auto& [blocks, vm_id] : vm_blocks)
        {
            // we create a new machine based off of the same settings to make things more annoying
            // but the same machine could be used :)
            virt::pidg::machine_ptr machine = virt::pidg::machine::create(settings);
            machine->add_block_context(block_labels);

            bool first = true;
            for (auto& translated_block : blocks)
            {
                asmb::code_container_ptr result_container = machine->lift_block(translated_block);
                if (first)
                {
                    asmb::code_label_ptr entry_mark = asmb::code_label::create();
                    result_container->bind_start(entry_mark);

                    entry_point = entry_mark;
                    first = false;
                }

                vm_section.add_code_container(result_container);
            }
        }

        // overwrite the original instructions
        uint32_t delete_size = vm_iat_calls[c + 1].first - vm_iat_calls[c].first;
        va_ran.emplace_back(parser.offset_to_rva(vm_iat_calls[c].first), delete_size);

        // incase jump goes to previous call, set it to nops
        va_nop.emplace_back(parser.offset_to_rva(vm_iat_calls[c + 1].first), call_size_64);

        // add vmenter for root block
        va_enters.emplace_back(parser.offset_to_rva(vm_iat_calls[c].first), entry_point);
    }

    std::printf("\n");

    auto& [code_section, code_section_bytes] = generator.add_section(".vmcode");
    code_section.PointerToRawData = last_section->PointerToRawData + last_section->SizeOfRawData;
    code_section.SizeOfRawData = 0;
    code_section.VirtualAddress = generator.align_section(last_section->VirtualAddress + last_section->Misc.VirtualSize);
    code_section.Misc.VirtualSize = generator.align_section(1);
    code_section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_CODE;
    code_section.PointerToRelocations = 0;
    code_section.NumberOfRelocations = 0;
    code_section.NumberOfLinenumbers = 0;

    codec::encoded_vec vm_code_bytes = vm_section.compile_section(code_section.VirtualAddress);
    code_section.SizeOfRawData = generator.align_file(vm_code_bytes.size());
    code_section.Misc.VirtualSize = generator.align_section(vm_code_bytes.size());
    code_section_bytes += vm_code_bytes;

    last_section = &code_section;

    // now that the section is compiled we must:
    // delete the code marked by va_delete
    // create jumps marked by va_enters

    std::vector<std::pair<uint32_t, std::vector<uint8_t>>> va_inserts;
    for (auto& [enter_va, enter_location] : va_enters)
    {
        codec::enc::req jump_request = encode(codec::m_jmp, ZIMMS(enter_location->get_address() - enter_va - 5));
        va_inserts.emplace_back(enter_va, codec::encode_request(jump_request));
    }

    generator.add_ignores(va_nop);
    generator.add_randoms(va_ran);
    generator.add_inserts(va_inserts);
    generator.bake_modifications();

    // custom entry point
    {
        pe::pe_packer packer(&generator);
        packer.set_overlay(false);

        asmb::section_manager packer_sm = packer.create_section();

        auto& [packer_section, packer_bytes] = generator.add_section(".pack");
        packer_section.PointerToRawData = last_section->PointerToRawData + last_section->SizeOfRawData;
        packer_section.SizeOfRawData = 0;
        packer_section.VirtualAddress = generator.align_section(last_section->VirtualAddress + last_section->Misc.VirtualSize);
        packer_section.Misc.VirtualSize = generator.align_section(1);
        packer_section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;
        packer_section.PointerToRelocations = 0;
        packer_section.NumberOfRelocations = 0;
        packer_section.NumberOfLinenumbers = 0;

        codec::encoded_vec packer_code_bytes = packer_sm.compile_section(packer_section.VirtualAddress);
        auto [packer_pdb_offset, size] = pe::pe_packer::insert_pdb(packer_code_bytes);

        packer_section.SizeOfRawData = generator.align_file(packer_code_bytes.size());
        packer_section.Misc.VirtualSize = generator.align_section(packer_code_bytes.size());
        packer_bytes += packer_code_bytes;

        generator.add_custom_pdb(
            packer_section.VirtualAddress + packer_pdb_offset,
            packer_section.PointerToRawData + packer_pdb_offset,
            size
        );
    }

    generator.save_file("EagleVMSandboxProtected.exe");
    std::printf("\n[+] generated output file -> EagleVMSandboxProtected.exe\n");

    return 0;
}
