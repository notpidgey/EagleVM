#pragma once
#include <Zydis/Zydis.h>

#define NUM_OF_VREGS 4
#define I_VIP 0
#define I_VSP 1
#define I_VREGS 2
#define I_VTEMP 3

enum class virtual_registers
{
	vip = 0,	//virtual instruction pointer
	vsp = 1,	//virtual stack pointer
	vregs = 2	//address of stack where scratch registers are located
};