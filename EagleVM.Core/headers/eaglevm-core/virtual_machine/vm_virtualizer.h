#pragma once
#include "eaglevm-core/disassembler/disassembler.h"
#include "eaglevm-core/disassembler/basic_block.h"

#include "eaglevm-core/virtual_machine/vm_inst.h"

namespace eagle::virt
{
    class vm_virtualizer
    {
    public:
        explicit vm_virtualizer(vm_inst* virtual_machine);
        ~vm_virtualizer();

        std::vector<asmb::function_container> virtualize_segment(dasm::segment_dasm* dasm);
        asmb::function_container virtualize_block(dasm::segment_dasm* dasm, dasm::basic_block* block);

        asmb::code_label* get_label(dasm::basic_block* block);

        static encoded_vec create_padding(size_t bytes);
        static encoded_vec create_jump(uint32_t rva, asmb::code_label* rva_target);

    private:
        std::unordered_map<dasm::basic_block*, asmb::code_label*> block_labels_;
        vm_inst* vm_inst_;
        vm_inst_handlers* hg_;
        vm_inst_regs* rm_;

        void call_vm_enter(asmb::function_container& container, asmb::code_label* target);
        void call_vm_exit(asmb::function_container& container, asmb::code_label* target);
        void create_vm_jump(zyids_mnemonic mnemonic, asmb::function_container &container, asmb::code_label* rva_target);

        std::pair<bool, asmb::function_container> translate_to_virtual(const zydis_decode& decoded_instruction, uint64_t original_rva);
    };
}