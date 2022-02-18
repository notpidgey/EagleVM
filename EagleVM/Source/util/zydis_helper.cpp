#include "util/zydis_helper.h"

void zydis_helper::setup_decoder()
{
    ZydisDecoderInit(&zyids_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
}

zydis_register zydis_helper::get_bit_version(zydis_register zy_register, register_size requested_size)
{
    auto index = 0;
    switch (zydis_helper::get_reg_size(zy_register))
    {
        case unsupported:
            return zy_register;
        case bit64:
            index = zy_register - ZYDIS_REGISTER_RAX;
            break;
        case bit32:
            index = zy_register - ZYDIS_REGISTER_EAX;
            break;
        case bit16:
            index = zy_register - ZYDIS_REGISTER_AX;
            break;
        case bit8:
            index = zy_register - ZYDIS_REGISTER_AL;
            break;
    }

    switch (requested_size)
    {
        case unsupported:
            break;
        case bit64:
            return zydis_register(ZYDIS_REGISTER_RAX + index);
        case bit32:
            return zydis_register(ZYDIS_REGISTER_EAX + index);
        case bit16:
            return zydis_register(ZYDIS_REGISTER_AX + index);
        case bit8:
            return zydis_register(ZYDIS_REGISTER_AL + index);
    }

    return zy_register;
}

register_size zydis_helper::get_reg_size(const zydis_register zy_register)
{
    if (zy_register >= ZYDIS_REGISTER_AL && zy_register <= ZYDIS_REGISTER_R15B)
        return bit8;

    if (zy_register >= ZYDIS_REGISTER_AX && zy_register <= ZYDIS_REGISTER_R15W)
        return bit16;

    if (zy_register >= ZYDIS_REGISTER_EAX && zy_register <= ZYDIS_REGISTER_R15D)
        return bit32;

    if (zy_register >= ZYDIS_REGISTER_RAX && zy_register <= ZYDIS_REGISTER_R15)
        return bit64;

    return unsupported;
}

std::vector<uint8_t> zydis_helper::encode_request(ZydisEncoderRequest& request)
{
    std::vector<uint8_t> instruction(ZYDIS_MAX_INSTRUCTION_LENGTH);
    ZyanUSize encoded_length = sizeof(uint8_t) * ZYDIS_MAX_INSTRUCTION_LENGTH;
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

void zydis_helper::add_op(ZydisEncoderRequest& req, zydis_eimm imm)
{
    auto op_index = req.operand_count;

    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    req.operands[op_index].imm = imm;
    req.operand_count++;
}

void zydis_helper::add_op(ZydisEncoderRequest& req, zydis_emem mem)
{
    auto op_index = req.operand_count;

    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_MEMORY;
    req.operands[op_index].mem = mem;
    req.operand_count++;
}

void zydis_helper::add_op(ZydisEncoderRequest& req, zydis_eptr ptr)
{
    auto op_index = req.operand_count;

    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_POINTER;
    req.operands[op_index].ptr = ptr;
    req.operand_count++;
}

void zydis_helper::add_op(ZydisEncoderRequest& req, zydis_ereg reg)
{
    auto op_index = req.operand_count;

    req.operands[op_index].type = ZYDIS_OPERAND_TYPE_REGISTER;
    req.operands[op_index].reg = reg;
    req.operand_count++;
}

std::vector<uint8_t> zydis_helper::encode_queue(std::vector<ZydisEncoderRequest>& queue)
{
    std::vector<uint8_t> data;
    for (auto& i : queue)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        ZydisEncoderEncodeInstruction(&i, instruction_data.data(), &encoded_length);
        instruction_data.resize(encoded_length);

        data.insert(data.end(), instruction_data.begin(), instruction_data.end());
    }

    return data;
}

std::vector<zydis_decode> zydis_helper::get_instructions(void* data, size_t size)
{
    std::vector<zydis_decode> decode_data;

    ZyanUSize offset = 0;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

    while (ZYAN_SUCCESS(
            ZydisDecoderDecodeFull(&zyids_decoder, (char*)data + offset, size - offset, &instruction, operands,
                    ZYDIS_MAX_OPERAND_COUNT_VISIBLE, ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY)))
    {
        zydis_decode decoded = {instruction, 0};
        std::memcpy(&decoded.operands[0], operands, sizeof operands);

        decode_data.push_back(decoded);
        offset += instruction.length;
    }

    return decode_data;
}