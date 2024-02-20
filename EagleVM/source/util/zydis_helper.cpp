#include "util/zydis_helper.h"

void zydis_helper::setup_decoder()
{
    ZydisDecoderInit(&zyids_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    ZydisFormatterInit(&zydis_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
}

zydis_register zydis_helper::get_bit_version(zydis_register zy_register, reg_size requested_size)
{
    auto index = 0;
    switch (get_reg_size(zy_register))
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
            if (zy_register >= ZYDIS_REGISTER_SPL)
                index = zy_register - ZYDIS_REGISTER_SPL + 4;
            else
                index = zy_register - ZYDIS_REGISTER_AL;
        break;
    }

    switch (requested_size)
    {
        case unsupported:
            break;
        case bit64:
            return static_cast<zydis_register>(ZYDIS_REGISTER_RAX + index);
        case bit32:
            return static_cast<zydis_register>(ZYDIS_REGISTER_EAX + index);
        case bit16:
            return static_cast<zydis_register>(ZYDIS_REGISTER_AX + index);
        case bit8:
            if (index >= 4)
                return static_cast<zydis_register>(ZYDIS_REGISTER_SPL + index - 4);
            else
                return static_cast<zydis_register>(ZYDIS_REGISTER_AL + index);
    }

    return zy_register;
}

reg_size zydis_helper::get_reg_size(const zydis_register zy_register)
{
    const uint16_t bit_size = ZydisRegisterGetWidth(ZYDIS_MACHINE_MODE_LONG_64, zy_register);
    return static_cast<reg_size>(bit_size / 8);
}

char zydis_helper::reg_size_to_string(reg_size reg_size)
{
    switch (reg_size)
    {
        case bit64:
            return 'Q';
        case bit32:
            return 'D';
        case bit16:
            return 'W';
        case bit8:
            return 'B';
        default:
            return '?';
    }
}

std::vector<uint8_t> zydis_helper::encode_request(ZydisEncoderRequest& request)
{
    std::vector<uint8_t> instruction(ZYDIS_MAX_INSTRUCTION_LENGTH);
    ZyanUSize encoded_length = sizeof(uint8_t) * ZYDIS_MAX_INSTRUCTION_LENGTH;
    ZyanStatus status = ZydisEncoderEncodeInstruction(&request, instruction.data(), &encoded_length);
    if(!ZYAN_SUCCESS(status))
    {
        __debugbreak();
    }

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

zydis_encoder_request zydis_helper::decode_to_encode(const zydis_decode& decode)
{
    zydis_encoder_request encode_request;
    ZydisEncoderDecodedInstructionToEncoderRequest(&decode.instruction, decode.operands, decode.instruction.operand_count, &encode_request);

    return encode_request;
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

std::string zydis_helper::instruction_to_string(const zydis_decode& decode)
{
    char buffer[256];
    ZydisFormatterFormatInstruction(&zydis_formatter,
        &decode.instruction,
        (ZydisDecodedOperand*)&decode.operands,
        decode.instruction.operand_count_visible,
        buffer, sizeof(buffer), 0x140000000, ZYAN_NULL);

    return std::string(buffer);
}

std::string zydis_helper::operand_to_string(const zydis_decode& decode, const int index)
{
    char buffer[256];
    ZydisFormatterFormatOperand(&zydis_formatter,
        &decode.instruction,
        &decode.operands[index],
        buffer, sizeof(buffer), 0x140000000, ZYAN_NULL);

    return std::string(buffer);
}

std::vector<uint8_t> zydis_helper::encode_queue(std::vector<ZydisEncoderRequest>& queue)
{
    std::vector<uint8_t> data;
    for (auto& i : queue)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        const ZyanStatus result = ZydisEncoderEncodeInstruction(&i, instruction_data.data(), &encoded_length);
        if(!ZYAN_SUCCESS(result))
        {
            __debugbreak();
        }

        instruction_data.resize(encoded_length);
        data.insert(data.end(), instruction_data.begin(), instruction_data.end());
    }

    return data;
}

std::vector<std::string> zydis_helper::print_queue(std::vector<zydis_encoder_request>& queue, uint32_t address)
{
    std::vector<std::string> data; 
    for (auto& instruction : queue)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        ZydisEncoderEncodeInstruction(&instruction, instruction_data.data(), &encoded_length);
        instruction_data.resize(encoded_length);
        
        std::stringstream format;
        format << ZydisMnemonicGetString(instruction.mnemonic) << " ";
        
        for (uint8_t byte : instruction_data)
            format << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";

        data.push_back(format.str());
    }

    return data;
}

std::vector<zydis_decode> zydis_helper::get_instructions(void* data, size_t size)
{
    std::vector<zydis_decode> decode_data;

    ZyanUSize offset = 0;
    zydis_decode decoded_instruction{};

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&zyids_decoder, (char*)data + offset, size - offset, &decoded_instruction.instruction, decoded_instruction.operands)))
    {
        decode_data.push_back(decoded_instruction);
        offset += decoded_instruction.instruction.length;
    }

    return decode_data;
}