#include "virtual_machine/vm_generator.h"

void vm_generator::init_vreg_map()
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

void vm_generator::init_ran_consts()
{
	//encryption
	//rol r15, key1
	//sub r15, key2
	//ror r15, key3

	//decryption
	//rol r15, key3
	//add r15, key2
	//ror r15, key1

	//this should be dynamic and more random.
	//in the future, each mnemonic should have a std::function definition and an opposite
	//this will allow for larger and more complex jmp dec/enc routines

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(1, UINT8_MAX);

	for (int i = 0; i < 3; i++)
		func_address_keys.values[i] = dist(rng);
}

void vm_generator::create_vm_enter()
{
	//vm enter routine:
	ZydisEncoderRequest encoder;

	// 1. push everything to stack
	handle_instructions vm_enter = vm_handle_generator::create_vm_enter();
	
	// 2. decrypt function jump address & jump
	std::vector encode_requests
	{
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, ZydisReg, ZydisMem>(ZREG(ZYDIS_REGISTER_R15), ZMEMBD(ZYDIS_REGISTER_RSP, -144, 8)),	//mov r15, [rsp-144]

		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ROL, ZydisReg, ZydisImm>(ZREG(ZYDIS_REGISTER_R15), ZIMMU(func_address_keys.values[2])),	//rol r15, key3
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_SUB, ZydisReg, ZydisImm>(ZREG(ZYDIS_REGISTER_R15), ZIMMU(func_address_keys.values[1])),	//add r15, key2
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ROR, ZydisReg, ZydisImm>(ZREG(ZYDIS_REGISTER_R15), ZIMMU(func_address_keys.values[0])),	//ror r15, key1

		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_JMP, ZydisReg>(ZREG(ZYDIS_REGISTER_R15)) //jmp r15
	};

	auto data = zydis_helper::encode_queue(encode_requests);

	for (int i = 0; i < data.size(); ++i)
		std::cout << std::hex << (int)data[i] << " ";
}