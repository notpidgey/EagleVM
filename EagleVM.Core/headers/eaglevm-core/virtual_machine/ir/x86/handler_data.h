#pragma once
#include <unordered_map>
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_include.h"

namespace eagle::ir
{
    extern std::unordered_map<codec::mnemonic, std::shared_ptr<handler::base_handler_gen>> instruction_handlers;
    extern std::unordered_map<
        codec::mnemonic,
        std::function<std::shared_ptr<lifter::base_x86_translator>(const std::shared_ptr<ir_translator>&, const codec::dec::inst_info&, uint64_t)>
    > instruction_lifters;
}
