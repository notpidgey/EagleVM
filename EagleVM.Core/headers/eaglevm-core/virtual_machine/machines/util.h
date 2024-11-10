#pragma once
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/util/assert.h"

#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::virt
{
    inline ir::ir_size to_ir_size(const codec::reg_size reg_size)
    {
        switch (reg_size)
        {
            case codec::empty:
                return ir::ir_size::none;
            case codec::bit_512:
                return ir::ir_size::bit_512;
            case codec::bit_256:
                return ir::ir_size::bit_256;
            case codec::bit_128:
                return ir::ir_size::bit_128;
            case codec::bit_64:
                return ir::ir_size::bit_64;
            case codec::bit_32:
                return ir::ir_size::bit_32;
            case codec::bit_16:
                return ir::ir_size::bit_16;
            case codec::bit_8:
                return ir::ir_size::bit_8;
        }

        VM_ASSERT("reached invalid ir size");
        return ir::ir_size::none;
    }

    inline codec::reg_size to_reg_size(const ir::ir_size ir_size)
    {
        switch (ir_size)
        {
            case ir::ir_size::bit_512:
                return codec::reg_size::bit_512;
            case ir::ir_size::bit_256:
                return codec::reg_size::bit_256;
            case ir::ir_size::bit_128:
                return codec::reg_size::bit_128;
            case ir::ir_size::bit_64:
                return codec::reg_size::bit_64;
            case ir::ir_size::bit_32:
                return codec::reg_size::bit_32;
            case ir::ir_size::bit_16:
                return codec::reg_size::bit_16;
            case ir::ir_size::bit_8:
                return codec::reg_size::bit_8;
            case ir::ir_size::none:
                return codec::reg_size::empty;
        }

        VM_ASSERT("reached invalid reg size");
        return codec::reg_size::empty;
    }
}
