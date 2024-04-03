#include "eaglevm-core/assembler/x86/zydis_helper.h"
#include "eaglevm-core/assembler/x86/zydis_defs.h"

namespace eagle::asmbl::x86
{
    void setup_decoder()
    {
        ZydisDecoderInit(&zyids_decoder, ZYDIS_MACHINE_MODE_LONG_64,
            ZYDIS_STACK_WIDTH_64);
        ZydisFormatterInit(&zydis_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
    }

    reg get_bit_version(reg input_reg, const reg_size target_size)
    {
        const zydis_register zy_register = static_cast<zydis_register>(input_reg);
        const zydis_reg_class zy_reg_class = static_cast<zydis_reg_class>(target_size);

        const uint8_t id = ZydisRegisterGetId(zy_register);
        zydis_register enc = ZydisRegisterEncode(zy_reg_class, id);

        return static_cast<reg>(enc);
    }

    bool is_upper_8(const reg reg)
    {
        switch (reg)
        {
            case ah:
            case bh:
            case ch:
            case dh:
                return true;
            default:
                return false;
        }
    }

    reg_size get_reg_size(const reg reg)
    {
        const zydis_register zy_register = static_cast<zydis_register>(reg);
        const uint16_t bit_size = ZydisRegisterGetWidth(ZYDIS_MACHINE_MODE_LONG_64, zy_register);

        return static_cast<reg_size>(bit_size / 8);
    }

    char reg_size_to_string(const reg_size reg_size)
    {
        switch (reg_size)
        {
            case gpr_64:
                return 'Q';
            case gpr_32:
                return 'D';
            case gpr_16:
                return 'W';
            case gpr_8:
                return 'B';
            default:
                return '?';
        }
    }

    std::vector<uint8_t> encode_request(const enc::req& request)
    {
        std::vector<uint8_t> instruction(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = sizeof(uint8_t) * ZYDIS_MAX_INSTRUCTION_LENGTH;
        ZyanStatus status = ZydisEncoderEncodeInstruction(
            &request, instruction.data(), &encoded_length);

        if (!ZYAN_SUCCESS(status))
            __debugbreak();

        instruction.resize(encoded_length);
        return instruction;
    }

    ZydisEncoderRequest create_encode_request(const mnemonic mnemonic)
    {
        ZydisEncoderRequest req;
        ZYAN_MEMSET(&req, 0, sizeof(req));
        req.mnemonic = static_cast<ZydisMnemonic>(mnemonic);
        req.operand_count = 0;

        return req;
    }

    enc::req decode_to_encode(const dec::inst_info& decode)
    {
        enc::req encode_request;
        ZydisEncoderDecodedInstructionToEncoderRequest(
            &decode.instruction, decode.operands,
            decode.instruction.operand_count_visible, &encode_request);

        return encode_request;
    }

    void add_op(ZydisEncoderRequest& req, const enc::op_imm imm)
    {
        auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
        req.operands[op_index].imm = imm;
        req.operand_count++;
    }

    void add_op(ZydisEncoderRequest& req, const enc::op_mem& mem)
    {
        auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_MEMORY;
        req.operands[op_index].mem = mem;
        req.operand_count++;
    }

    void add_op(ZydisEncoderRequest& req, const enc::op_ptr ptr)
    {
        auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_POINTER;
        req.operands[op_index].ptr = ptr;
        req.operand_count++;
    }

    void add_op(ZydisEncoderRequest& req, enc::op_reg reg)
    {
        auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_REGISTER;
        req.operands[op_index].reg = reg;
        req.operand_count++;
    }

    std::string instruction_to_string(const dec::inst_info& decode)
    {
        char buffer[256];
        ZydisFormatterFormatInstruction(&zydis_formatter,
            &decode.instruction,
            (ZydisDecodedOperand*)&decode.operands,
            decode.instruction.operand_count_visible,
            buffer, sizeof(buffer), 0x140000000,
            ZYAN_NULL);

        return std::string(buffer);
    }

    std::string operand_to_string(const dec::inst_info& decode, const int index)
    {
        char buffer[256];
        ZydisFormatterFormatOperand(&zydis_formatter,
            &decode.instruction,
            &decode.operands[index],
            buffer, sizeof(buffer), 0x140000000, ZYAN_NULL);

        return std::string(buffer);
    }

    std::vector<uint8_t> compile(const enc::req& request)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        const ZyanStatus result = ZydisEncoderEncodeInstruction(
            &request, instruction_data.data(), &encoded_length);
        if (!ZYAN_SUCCESS(result))
        {
            __debugbreak();
        }

        instruction_data.resize(encoded_length);
        return instruction_data;
    }

    std::vector<uint8_t> compile_absolute(enc::req& request, uint32_t address)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        const ZyanStatus result = ZydisEncoderEncodeInstructionAbsolute(
            &request, instruction_data.data(),
            &encoded_length, address);
        if (!ZYAN_SUCCESS(result))
        {
            __debugbreak();
        }

        instruction_data.resize(encoded_length);
        return instruction_data;
    }

    std::vector<uint8_t> compile_queue(std::vector<ZydisEncoderRequest>& queue)
    {
        std::vector<uint8_t> data;
        for (auto& i : queue)
        {
            std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
            ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

            const ZyanStatus result = ZydisEncoderEncodeInstruction(
                &i, instruction_data.data(), &encoded_length);
            if (!ZYAN_SUCCESS(result))
            {
                __debugbreak();
            }

            instruction_data.resize(encoded_length);
            data.insert(data.end(), instruction_data.begin(), instruction_data.end());
        }

        return data;
    }

    std::vector<uint8_t> compile_queue_absolute(std::vector<enc::req>& queue)
    {
        std::vector<uint8_t> data;

        uint64_t current_rva = 0;
        for (auto& i : queue)
        {
            std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
            ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

            const ZyanStatus result = ZydisEncoderEncodeInstructionAbsolute(
                &i, instruction_data.data(), &encoded_length, current_rva);
            if (!ZYAN_SUCCESS(result))
            {
                __debugbreak();
            }

            instruction_data.resize(encoded_length);
            data.insert(data.end(), instruction_data.begin(), instruction_data.end());

            current_rva += encoded_length;
        }

        return data;
    }

    std::vector<std::string> print_queue(std::vector<enc::req>& queue,
        uint32_t address)
    {
        std::vector<std::string> data;
        for (auto& instruction : queue)
        {
            std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
            ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

            ZydisEncoderEncodeInstruction(&instruction, instruction_data.data(),
                &encoded_length);
            instruction_data.resize(encoded_length);

            std::stringstream format;
            format << ZydisMnemonicGetString(instruction.mnemonic) << " ";

            for (uint8_t byte : instruction_data)
                format << std::hex << std::setw(2) << std::setfill('0') << static_cast<
                    int>(byte) << " ";

            data.push_back(format.str());
        }

        return data;
    }

    bool has_relative_operand(dec::inst_info& decode)
    {
        const auto& [instruction, operands] = decode;
        for (int i = 0; i < instruction.operand_count; i++)
        {
            const dec::operand& operand = operands[i];
            switch (operand.type)
            {
                case ZYDIS_OPERAND_TYPE_MEMORY:
                {
                    if (operand.mem.base == ZYDIS_REGISTER_RIP)
                        return true;

                    if (operand.mem.base == ZYDIS_REGISTER_NONE && operand.mem.index ==
                        ZYDIS_REGISTER_NONE)
                        return true;

                    break;
                }
                case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                {
                    if (operand.imm.is_relative)
                        return true;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        return false;
    }

    std::pair<uint64_t, uint8_t> calc_relative_rva(const dec::inst_info& decode,
        const uint32_t rva,
        const int8_t operand)
    {
        const auto& [instruction, operands] = decode;

        uint64_t target_address = rva;
        if (operand < 0)
        {
            for (int i = 0; i < instruction.operand_count; i++)
            {
                auto result = ZydisCalcAbsoluteAddress(&instruction, &operands[i], rva,
                    &target_address);
                if (result == ZYAN_STATUS_SUCCESS)
                    return {target_address, i};
            }
        }
        else
        {
            auto result = ZydisCalcAbsoluteAddress(&instruction, &operands[operand],
                rva, &target_address);
            if (result == ZYAN_STATUS_SUCCESS)
                return {target_address, operand};
        }

        return {target_address, -1};
    }

    std::vector<dec::inst_info> get_instructions(void* data, size_t size)
    {
        std::vector<dec::inst_info> decode_data;

        ZyanUSize offset = 0;
        dec::inst_info decoded_instruction{};

        while (ZYAN_SUCCESS(
            ZydisDecoderDecodeFull(&zyids_decoder, (char*)data + offset, size - offset
                , &decoded_instruction.instruction
                , decoded_instruction.operands
            )))
        {
            decode_data.push_back(decoded_instruction);
            offset += decoded_instruction.instruction.length;
        }

        return decode_data;
    }
}
