#include "eaglevm-core/virtual_machine/machines/base_machine.h"

namespace eagle::machine
{
    void base_machine::add_translator(const il::command_type cmd, const translate_func& func)
    {
        translators[cmd] = func;
    }

    translate_func base_machine::get_translator(const il::command_type cmd)
    {
        if(translators.contains(cmd))
            return translators[cmd];

        return nullptr;
    }

}