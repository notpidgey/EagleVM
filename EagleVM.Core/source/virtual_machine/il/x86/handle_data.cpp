#include "eaglevm-core/virtual_machine/il/x86/handler_data.h"

namespace eagle::il
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
}
