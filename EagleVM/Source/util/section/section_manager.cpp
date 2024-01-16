#include "util/section/section_manager.h"

void section_manager::bake_section(uint32_t section_address)
{
    uint32_t current_address = section_address;

    // this should take all the functions in the section and connect them to desired labels
    for (function_container& sec_function : section_functions)
    {

    }
}
