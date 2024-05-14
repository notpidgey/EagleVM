#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

#define CREATE_LIFTER_GEN(x) [](codec::dec::inst_info decode, const uint64_t rva) \
{ return std::static_pointer_cast<lifter::base_x86_lifter>(std::make_shared<lifter::x>(decode, rva)); }

namespace eagle::ir
{
    std::unordered_map<codec::mnemonic, std::shared_ptr<handler::base_handler_gen>> instruction_handlers =
    {
        { codec::m_add, std::make_shared<handler::add>() },
        { codec::m_cmp, std::make_shared<handler::cmp>() },
        { codec::m_dec, std::make_shared<handler::dec>() },
        { codec::m_imul, std::make_shared<handler::imul>() },
        { codec::m_inc, std::make_shared<handler::inc>() },
        { codec::m_lea, std::make_shared<handler::lea>() },
        { codec::m_mov, std::make_shared<handler::mov>() },
        { codec::m_movsx, std::make_shared<handler::movsx>() },
        { codec::m_pop, std::make_shared<handler::pop>() },
        { codec::m_push, std::make_shared<handler::push>() },
        { codec::m_sub, std::make_shared<handler::sub>() },
    };

    std::unordered_map<
        codec::mnemonic,
        std::function<std::shared_ptr<lifter::base_x86_translator>(codec::dec::inst_info, uint64_t)>
    >
    instruction_lifters =
    {
        { codec::m_add, CREATE_LIFTER_GEN(add) },
        { codec::m_cmp, CREATE_LIFTER_GEN(cmp) },
        { codec::m_dec, CREATE_LIFTER_GEN(dec) },
        { codec::m_imul, CREATE_LIFTER_GEN(imul) },
        { codec::m_inc, CREATE_LIFTER_GEN(inc) },
        { codec::m_lea, CREATE_LIFTER_GEN(lea) },
        { codec::m_mov, CREATE_LIFTER_GEN(mov) },
        { codec::m_movsx, CREATE_LIFTER_GEN(movsx) },
        { codec::m_pop, CREATE_LIFTER_GEN(pop) },
        { codec::m_push, CREATE_LIFTER_GEN(push) },
        { codec::m_sub, CREATE_LIFTER_GEN(sub) },
    };
}
