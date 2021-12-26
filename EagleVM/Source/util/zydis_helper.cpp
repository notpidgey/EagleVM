#include "util/zydis_helper.h"

std::vector<uint8_t> zydis_helper::encode_request(ZydisEncoderRequest& request)
{
    std::vector<uint8_t> instruction(ZYDIS_MAX_INSTRUCTION_LENGTH);
    ZyanUSize encoded_length = sizeof uint8_t * ZYDIS_MAX_INSTRUCTION_LENGTH;
    ZydisEncoderEncodeInstruction(&request, instruction.data(), &encoded_length);

    instruction.resize(encoded_length);

    return instruction;
}

ZydisEncoderRequest zydis_helper::create_encode_request(const ZydisMnemonic mnemonic, const int operand_count)
{
    ZydisEncoderRequest req;
    ZYAN_MEMSET(&req, 0, sizeof(req));
    req.mnemonic = mnemonic;
    req.operand_count = operand_count;

    return req;
}

std::vector<uint8_t> zydis_helper::encode_instruction_0(ZydisMnemonic mnemonic)
{
    ZydisEncoderRequest req = create_encode_request(mnemonic, 0);

    return encode_request(req);
}

std::vector<uint8_t> zydis_helper::encode_instruction_1(const ZydisMnemonic mnemonic, ZydisImm imm)
{
    ZydisEncoderRequest req = create_encode_request(mnemonic, 1);
    req.operands[0].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    req.operands[0].imm = imm;

    return encode_request(req);
}

std::vector<uint8_t> zydis_helper::encode_instruction_1(const ZydisMnemonic mnemonic, ZydisMem mem)
{
    ZydisEncoderRequest req = create_encode_request(mnemonic, 1);
    req.operands[0].type = ZYDIS_OPERAND_TYPE_MEMORY;
    req.operands[0].mem = mem;

    return encode_request(req);
}

std::vector<uint8_t> zydis_helper::encode_instruction_1(const ZydisMnemonic mnemonic, ZydisPtr ptr)
{
    ZydisEncoderRequest req = create_encode_request(mnemonic, 1);
    req.operands[0].type = ZYDIS_OPERAND_TYPE_POINTER;
    req.operands[0].ptr = ptr;

    return encode_request(req);
}

std::vector<uint8_t> zydis_helper::encode_instruction_1(const ZydisMnemonic mnemonic, ZydisReg reg)
{
    ZydisEncoderRequest req = create_encode_request(mnemonic, 1);
    req.operands[0].type = ZYDIS_OPERAND_TYPE_REGISTER;
    req.operands[0].reg = reg;

    return encode_request(req);
}
