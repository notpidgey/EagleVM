#pragma once

enum handler_override : uint8_t
{
    ho_default = 0,

    /*
     * vmstore override
     */

    /*
     * P1 VTEMP: displacement
     * P2 STACK: reg value
     */
    ho_vmstore_temp_stack = 0,

    /*
     * vmload override
     */

    /*
     * P1 VTEMP: displacement
     * RESULT STACK: reg value
     */
    ho_vmload_temp_stack = 0,
};