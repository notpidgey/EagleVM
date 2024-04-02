#pragma once
#include <tuple>
#include <utility>

#include "eaglevm-core/assembler/function_container.h"
#include "eaglevm-core/virtual_machine/il/il_bb.h"

namespace eagle::il::translator
{
    enum class translate_status
    {
        success,
        unsupported
    };

    enum translate_op_action
    {
        action_none = 0b0,

        // 64 bit address of operand
        action_address = 0b1,

        // value of operand
        action_value = 0b10,

        // VREGS displacement of target register
        action_reg_offset = 0b100,
    };

    class base_x86_translator
    {
    public:
        virtual ~base_x86_translator() = default;
        explicit base_x86_translator(il_bb_ptr block_ptr, zydis_decode decode);
        explicit base_x86_translator(zydis_decode decode);

        virtual bool translate_to_il(uint64_t original_rva);
        virtual int get_op_action(zyids_operand_t op_type, int index);

    protected:
        il_bb_ptr block;

        zydis_decoded_inst inst;
        zydis_decoded_operand operands[ZYDIS_MAX_OPERAND_COUNT];

        uint64_t stack_displacement;

        virtual translate_status encode_operand(zydis_dreg op_reg, uint8_t idx);
        virtual translate_status encode_operand(zydis_dmem op_mem, uint8_t idx);
        virtual translate_status encode_operand(zydis_dptr op_ptr, uint8_t idx);
        virtual translate_status encode_operand(zydis_dimm op_imm, uint8_t idx);

        virtual void finalize_translate_to_virtual();

        void load_reg_address(zydis_dreg reg);
        void load_reg_offset(zydis_dreg reg);
        void load_reg_value(zydis_dreg reg);
    };
}
