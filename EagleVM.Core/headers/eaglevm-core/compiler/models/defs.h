#pragma once
#include <vector>
#include <functional>
#include <cstdint>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/compiler/label_tracker.h"

namespace eagle::asmb
{
    using recompile_promise = std::function<codec::enc::req(uint64_t offset, label_tracker_ptr tracker)>;
    using recompile_chunk = std::function<std::vector<uint8_t>(uint64_t offset, label_tracker_ptr tracker)>;
    using dynamic_instruction = std::variant<codec::enc::req, recompile_promise, recompile_chunk>;

    using dynamic_inst_vec = std::vector<dynamic_instruction>;
    using inst_vec = std::vector<codec::enc::req>;
}