#pragma once
#include <tuple>
#include <utility>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir::lifter
{
    enum class translate_status
    {
        success,
        unsupported
    };

    class base_x86_translator
    {
    public:
        virtual ~base_x86_translator() = default;
        explicit base_x86_translator(codec::dec::inst_info decode, uint64_t rva);

        virtual bool translate_to_il(uint64_t original_rva);
        block_il_ptr get_block();

    protected:
        block_il_ptr block;
        uint64_t orig_rva;

        codec::dec::inst inst;
        codec::dec::operand operands[ZYDIS_MAX_OPERAND_COUNT];

        uint64_t stack_displacement = 0;

        virtual bool virtualize_as_address(codec::dec::operand operand, uint8_t idx);
        virtual translate_status encode_operand(codec::dec::op_reg op_reg, uint8_t idx);
        virtual translate_status encode_operand(codec::dec::op_mem op_mem, uint8_t idx);
        virtual translate_status encode_operand(codec::dec::op_ptr op_ptr, uint8_t idx);
        virtual translate_status encode_operand(codec::dec::op_imm op_mem, uint8_t idx);

        virtual void finalize_translate_to_virtual();
        virtual bool skip(uint8_t idx);

        il_size get_op_width() const;
    };
}
