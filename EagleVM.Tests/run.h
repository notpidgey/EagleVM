#pragma once
#include <Windows.h>

extern "C" {
    __cdecl void run_shellcode(uint8_t* shellcode_rip, CONTEXT* context);
}