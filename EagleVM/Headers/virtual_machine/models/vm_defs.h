#pragma once
#include <Zydis/Zydis.h>
#include "util/zydis_defs.h"

#define NUM_OF_VREGS 4
#define I_VIP 0
#define I_VSP 1
#define I_VREGS 2
#define I_VTEMP 3

#define MNEMONIC_VM_ENTER 0
#define MNEMONIC_VM_EXIT 1
#define MNEMONIC_VM_LOAD_REG 2