#include "disassembler/disassembler.h"

segment_disassembler::segment_disassembler(instructions_vec& segment, uint32_t binary_rva)
    : root_block(nullptr)
{
    function = segment;
    rva = binary_rva;
}

void segment_disassembler::generate_blocks()
{
    for(int i = 0; i < function.size(); i++)
    {
        zydis_encoder_request& inst = function[i];
        
    }
}