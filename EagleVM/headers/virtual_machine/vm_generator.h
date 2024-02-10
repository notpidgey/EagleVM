#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <intrin.h>

#include <Zydis/Zydis.h>

#include "virtual_machine/models/vm_defs.h"
#include "handlers/vm_handler_generator.h"
#include "virtual_machine/vm_register_manager.h"

#include "util/section/section_manager.h"
#include "util/util.h"

class vm_generator
{
public:
    vm_generator();

    void init_reg_order();
    void init_ran_consts();

    section_manager generate_vm_handlers(bool randomize_handler_position);

    void call_vm_enter(function_container& container, code_label* target);
    void call_vm_exit(function_container& container, code_label* target);
    static encoded_vec create_jump(uint32_t current, code_label* target);

    std::pair<bool, function_container> translate_to_virtual(const zydis_decode& decoded_instruction);
    static std::vector<uint8_t> create_padding(size_t bytes);

private:
    vm_register_manager rm_;
    vm_handler_generator hg_;
};
