#include <algorithm>

#include "pe/pe_parser.h"
#include "pe/pe_generator.h"
#include "virtual_machine/vm_generator.h"

#include "obfuscation/mba/mba.h"

int main(int argc, char* argv[])
{
    // this is definitely a work in progress
    // also, this is not a true way of generating mba expressions
    // all this does is use mba rewrites as operators can be rewritten with equivalent expressions
    // TODO: insertion of identities from "Defeating MBA-based Obfuscation"
    // mba_gen mba = mba_gen<std::uint64_t>(std::numeric_limits<uint64_t>::digits);

    // std::string result = mba.create_tree(op_plus, 5, 3);
    // std::cout << "\n[Final] " << result  << std::endl;

    auto executable = argc > 1 ? argv[1] : "EagleVMSandbox.exe";
    pe_parser parser = pe_parser(executable);
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
    parser.enum_imports
        (
            [&i](const PIMAGE_IMPORT_DESCRIPTOR import_descriptor, const PIMAGE_THUNK_DATA thunk_data,
                const PIMAGE_SECTION_HEADER import_section, int, const uint8_t* data_base)
                {
                    const uint8_t* import_section_raw = data_base + import_section->PointerToRawData;
                    const uint8_t* import_library = const_cast<uint8_t*>(import_section_raw + (import_descriptor->Name -
                                                                                         import_section->VirtualAddress));
                    const uint8_t* import_name = const_cast<uint8_t*>(import_section_raw + (thunk_data->u1.AddressOfData -
                                                                                      import_section->VirtualAddress + 2));

                    std::printf("%3i %-20s %-s\n", i, import_library, import_name);
                    i++;
                });

    std::printf("\n[+] porting original pe properties to new executable...\n");

    auto nt_header = parser.get_nt_header();
    std::printf("[>] image base -> %I64d bytes\n", nt_header->OptionalHeader.ImageBase);
    std::printf("[>] section alignment -> %lu bytes\n", nt_header->OptionalHeader.SectionAlignment);
    std::printf("[>] file alignment -> %lu bytes\n", nt_header->OptionalHeader.FileAlignment);
    std::printf("[>] stack reserve -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfStackReserve);
    std::printf("[>] stack commit -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfStackCommit);
    std::printf("[>] heap reserve -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfHeapReserve);
    std::printf("[>] heap commit -> %I64d bytes\n", nt_header->OptionalHeader.SizeOfHeapCommit);

    std::printf("\n[>] searching for uses of vm macros...\n");

    i = 1;
    std::vector<std::pair<uint32_t, stub_import>> vm_iat_calls = parser.find_iat_calls();
    std::printf("%3s %-10s %-10s\n", "", "rva", "vm");
    std::ranges::for_each
        (
            vm_iat_calls.begin(), vm_iat_calls.end(),
            [&i, &parser](const auto call)
                {
                    auto[file_offset, stub_import] = call;
                    switch (stub_import)
                    {
                        case stub_import::vm_begin:
                            std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "vm_begin");
                            break;
                        case stub_import::vm_end:
                            std::printf("%3i %-10i %-s\n", i, parser.offset_to_rva(file_offset), "vm_end");
                            break;
                        case stub_import::unknown:
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
    stub_import previous_call = stub_import::vm_end;
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
    else
    {
        std::printf("[+] successfully verified macro usage\n");
    }

    //to keep relative jumps of the image intact, it is best to just stick the vm section at the back of the pe
    pe_generator generator(&parser);
    generator.load_parser();

    IMAGE_SECTION_HEADER* last_section = &std::get<0>(generator.get_last_section());

    // its not a great idea to split up the virtual machines into a different section than the virtualized code
    // as it will aid reverse engineers in understanding what is happening in the binary
    auto& [vm_section, vm_section_bytes] = generator.add_section(".vmdata");
    vm_section.PointerToRawData = generator.align_file(last_section->PointerToRawData + last_section->SizeOfRawData);
    vm_section.SizeOfRawData = 0;
    vm_section.VirtualAddress = generator.align_section(last_section->VirtualAddress + last_section->Misc.VirtualSize);
    vm_section.Misc.VirtualSize = 0;
    vm_section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;
    vm_section.PointerToRelocations = 0;
    vm_section.NumberOfRelocations = 0;
    vm_section.NumberOfLinenumbers = 0;

    last_section = &vm_section;

    vm_generator vm_generator;
    vm_generator.init_reg_order();
    std::printf("[+] initialized random registers\n");

    vm_generator.init_ran_consts();
    std::printf("[+] created random constants\n\n");

    std::printf("[>] generating vm handlers at %04X...\n", (uint32_t)vm_section.VirtualAddress);
    
    section_manager vm_section_manager = vm_generator.generate_vm_handlers(false);
    encoded_vec vm_handlers_bytes = vm_section_manager.compile_section(vm_section.VirtualAddress);

    vm_section.SizeOfRawData = generator.align_file(vm_handlers_bytes.size());
    vm_section.Misc.VirtualSize = generator.align_section(vm_handlers_bytes.size());
    vm_handlers_bytes += vm_handlers_bytes;

    // now that we have all the vm handlers generated, we need to randomize them in the section
    // we need to create a map of all the handlers

    std::printf("\n[>] generating virtualized code...\n\n");
    for (int c = 0; c < vm_iat_calls.size(); c += 2) // i1 = vm_begin, i2 = vm_end
    {
        pe_protected_section protect_section = parser.offset_to_ptr(vm_iat_calls[c].first, vm_iat_calls[c + 1].first);
        std::vector<zydis_decode> instructions = zydis_helper::get_instructions
            (
                protect_section.instruction_protect_begin, protect_section.get_instruction_size()
            );

        std::printf("[+] function %i-%i\n", c, c + 1);
        std::printf("[>] instruction begin %u\n", parser.offset_to_rva(vm_iat_calls[c].first));
        std::printf("[>] instruction end %u\n", parser.offset_to_rva(vm_iat_calls[c + 1].first));
        std::printf("[>] instruction size %zu\n", protect_section.get_instruction_size());

        std::printf("[+] generated instructions\n\n");

        std::vector<zydis_encoder_request> section_instructions;

        bool currently_in_vm = false;
        std::ranges::for_each(instructions,
            [&](const zydis_decode instruction)
                {
                    auto [successfully_virtualized, instructions] = vm_generator.translate_to_virtual(instruction);
                    if (successfully_virtualized)
                    {                        
                        // check if we are already inside of virtual machine to prevent multiple enters
                        if (!currently_in_vm)
                        {
                            std::printf("\n[+] vmenter\n");

                            // call into the virtual machine
                            std::vector<zydis_encoder_request> enter_instructions = vm_generator.call_vm_enter();
                            section_instructions.insert(section_instructions.end(), enter_instructions.begin(), enter_instructions.end());

                            currently_in_vm = true;
                        }
                    }
                    else
                    {
                        // exit virtual machine if this is a non-virtual instruction
                        if (currently_in_vm)
                        {
                            // instruction is not supported by the virtual machine so we exit
                            std::printf("[-] vmexit\n\n");

                            // call out of the virtual machine
                            std::vector<zydis_encoder_request> exit_instructions = vm_generator.call_vm_exit();
                            section_instructions.insert(section_instructions.end(), exit_instructions.begin(), exit_instructions.end());

                            currently_in_vm = false;
                        }
                    }

                    // std::vector<std::string> messages = zydis_helper::print_queue(instructions, parser.offset_to_rva(vm_iat_calls[c].first));
                    // for(auto& message : messages)
                    //     std::printf("[>] %s\n", message.c_str());

                    // append all instructions to the section ( virtual or not )
                    //section_instructions.insert(section_instructions.end(), instructions.begin(), instructions.end());
                });

        // TODO: generate function return to original execution

        // result for each section should appear something like this:
        // [>] ftst                             example ( or any unsupported instruction )
        // [>] mov [rsp - 8], vmenter_const     prepare generated constant to be decrypted so that the vm can be entered
        // [>] jmp vmenter
        //
        //  virtual machine code
        //
        // [>] jmp vmend                        exit virtual machine due to unsupported instruction
        // [>] ftst                             example ( or any unsupported instruction )
        //
    }

    auto& [code_section, _1] = generator.add_section(".vmcode");
    code_section.PointerToRawData = last_section->PointerToRawData + last_section->SizeOfRawData;
    code_section.SizeOfRawData = 0;
    code_section.VirtualAddress = generator.align_section(last_section->VirtualAddress + last_section->Misc.VirtualSize);
    code_section.Misc.VirtualSize = generator.align_section(1);
    code_section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;
    code_section.PointerToRelocations = 0;
    code_section.NumberOfRelocations = 0;
    code_section.NumberOfLinenumbers = 0;

    last_section = &code_section;

    auto& [packer_section, _2] = generator.add_section(".pack");
    packer_section.PointerToRawData = last_section->PointerToRawData + last_section->SizeOfRawData;
    packer_section.SizeOfRawData = 0;
    packer_section.VirtualAddress = generator.align_section(last_section->VirtualAddress + last_section->Misc.VirtualSize);
    packer_section.Misc.VirtualSize = generator.align_section(1);
    packer_section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;
    packer_section.PointerToRelocations = 0;
    packer_section.NumberOfRelocations = 0;
    packer_section.NumberOfLinenumbers = 0;

    last_section = &packer_section;
    generator.save_file("box.exe");

    return 0;
}
