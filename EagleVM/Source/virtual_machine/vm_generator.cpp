#include "virtual_machine/vm_generator.h"

void vm_generator::create_vreg_map()
{
	//vreg_mapping is a table which contains definitions for real -> virtual registers
	//to create vm obfsucation, the registers need to be swapped on vm entry as follows

	//initialize random_registers with default register values
	std::vector<short> random_registers;
	for (int i = ZYDIS_REGISTER_RAX; i <= ZYDIS_REGISTER_R15; i++)
		random_registers.push_back(i);

	//randomize order
	auto rng = std::default_random_engine{};
	std::shuffle(random_registers.begin(), random_registers.end(), rng);

	//set random regsters ex. rax -> rsp
	for (int i = ZYDIS_REGISTER_RAX; i <= ZYDIS_REGISTER_R15; i++)
		vreg_mapping_[i] = random_registers[i - ZYDIS_REGISTER_RAX];
}

void vm_generator::create_vm_enter()
{
	//vm enter routine:
	ZydisEncoderRequest encoder;
	
	//
	// 1. push everything to stack
	//
	handle_instructions vm_enter = vm_handle_generator::create_vm_enter();

	//
	// 2. decode function jump
	//

	//
	// 3. jump
	//
	encoder = zydis_helper::create_encode_request<ZydisReg, ZydisReg>(ZYDIS_MNEMONIC_MOV, ZREG(ZYDIS_REGISTER_R15), ZREG(ZYDIS_REGISTER_R14));

	encoder = zydis_helper::create_encode_request(ZYDIS_MNEMONIC_JMP);
	zydis_helper::add_reg(encoder, ZREG(ZYDIS_REGISTER_R15));
}