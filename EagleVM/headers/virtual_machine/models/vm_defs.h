#pragma once
#include <Zydis/Zydis.h>
#include "util/zydis_defs.h"

#define NUM_OF_VREGS 7
#define I_VIP 0
#define I_VSP 1
#define I_VREGS 2

#define I_VTEMP 3
#define I_VTEMP2 4

#define I_VCALLSTACK 5
#define I_VCSRET 6

#define MNEMONIC_VM_ENTER 0
#define MNEMONIC_VM_EXIT 1
#define MNEMONIC_VM_LOAD_REG 2
#define MNEMONIC_VM_STORE_REG 3
#define MNEMONIC_VM_POP_RFLAGS 4
#define MNEMONIC_VM_PUSH_RFLAGS 5
#define MNEMONIC_VM_TRASH_RFLAGS 6