#include "util/zydis_helper.h"

std::vector<uint8_t> zydis_helper::encode_request(ZydisEncoderRequest& request)
{
    std::vector<uint8_t> instruction(ZYDIS_MAX_INSTRUCTION_LENGTH);
    ZyanUSize encoded_length = sizeof uint8_t * ZYDIS_MAX_INSTRUCTION_LENGTH;
    ZydisEncoderEncodeInstruction(&request, instruction.data(), &encoded_length);

    instruction.resize(encoded_length);

    return instruction;
}

ZydisEncoderRequest zydis_helper::create_encode_request(const ZydisMnemonic mnemonic)
{
    ZydisEncoderRequest req;
    ZYAN_MEMSET(&req, 0, sizeof(req));
    req.mnemonic = mnemonic;
    req.operand_count = 0;

    return req;
}

ZydisEncoderRequest& zydis_helper::add_imm(ZydisEncoderRequest& req, ZydisImm imm)
{
    auto op_index = req.operand_count;

    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    req.operands[op_index].imm = imm;
    req.operand_count++;

    return req;
}

ZydisEncoderRequest& zydis_helper::add_mem(ZydisEncoderRequest& req, ZydisMem mem)
{
    auto op_index = req.operand_count;

    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_MEMORY;
    req.operands[op_index].mem = mem;
    req.operand_count++;

    return req;
}

ZydisEncoderRequest& zydis_helper::add_ptr(ZydisEncoderRequest& req, ZydisPtr ptr)
{
    auto op_index = req.operand_count;
    
    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_POINTER;
    req.operands[op_index].ptr = ptr;
    req.operand_count++;

    return req;
}

ZydisEncoderRequest& zydis_helper::add_reg(ZydisEncoderRequest& req, ZydisReg reg)
{
    auto op_index = req.operand_count;
    
    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_REGISTER;
    req.operands[op_index].reg = reg;
    req.operand_count++;

    return req;
}
