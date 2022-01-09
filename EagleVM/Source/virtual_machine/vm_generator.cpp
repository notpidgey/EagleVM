#include "virtual_machine/vm_generator.h"

vm_generator::vm_generator()
{
	zydis_helper::setup_decoder();
}

void vm_generator::init_vreg_map()
{
	//initialize random_registers with default register values
	reg_order_.clear();
	for (int i = ZYDIS_REGISTER_RAX; i <= ZYDIS_REGISTER_R15; i++)
		reg_order_.push_back(i);

	//randomize order
	auto rng = std::default_random_engine{};
	std::shuffle(reg_order_.begin(), reg_order_.end(), rng);
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

std::vector<uint8_t> vm_generator::create_padding(size_t bytes)
{
	std::vector<uint8_t> padding(bytes);

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT8_MAX);

	for (int i = 0; i < bytes; i++)
		padding.push_back(dist(rng));

	return padding;
}

std::vector<uint8_t> vm_generator::create_vm_enter_jump(uint32_t va_protected)
{
	//Encryption
	va_protected = _rotl(va_protected, func_address_keys.values[0]);
	va_protected -= func_address_keys.values[1];
	va_protected = _rotr(va_protected, func_address_keys.values[2]);

	//Decryption
	//va_unprotect = _rotl(va_protected, func_address_keys.values[2]);
	//va_unprotect += func_address_keys.values[1];
	//va_unprotect = _rotr(va_unprotect, func_address_keys.values[0]);

	std::vector encode_requests
	{
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_PUSH, ZydisImm>(ZIMMU(va_protected)),	//push x
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_JMP,  ZydisImm>(ZIMMU(vm_enter_va)),		//jmp vm_enter
	};
	std::vector<uint8_t> data = zydis_helper::encode_queue(encode_requests);

	for (int i = 0; i < data.size(); ++i)
		std::cout << std::hex << (int)data[i] << " ";

	return data;
}

std::vector<uint8_t> vm_generator::create_vm_enter()
{
	//vm enter routine:
	// 1. push everything to stack
	const std::vector vm_enter = vm_handle_generator::create_vm_enter();
	
	// 2. decrypt function jump address & jump
	const std::vector encode_requests
	{
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_MOV, ZydisReg, ZydisMem>(ZREG(ZYDIS_REGISTER_R15), ZMEMBD(ZYDIS_REGISTER_RSP, -144, 8)),	//mov r15, [rsp-144]

		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ROL, ZydisReg, ZydisImm>(ZREG(ZYDIS_REGISTER_R15), ZIMMU(func_address_keys.values[2])),	//rol r15, key3
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ADD, ZydisReg, ZydisImm>(ZREG(ZYDIS_REGISTER_R15), ZIMMU(func_address_keys.values[1])),	//add r15, key2
		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_ROR, ZydisReg, ZydisImm>(ZREG(ZYDIS_REGISTER_R15), ZIMMU(func_address_keys.values[0])),	//ror r15, key1

		zydis_helper::create_encode_request<ZYDIS_MNEMONIC_JMP, ZydisReg>(ZREG(ZYDIS_REGISTER_R15))													//jmp r15
	};

	auto vm_enter_queue = zydis_helper::combine_vec(vm_enter, encode_requests);
	return zydis_helper::encode_queue(vm_enter_queue);
}