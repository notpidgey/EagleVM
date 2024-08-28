#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/codec/zydis_defs.h"

#include "eaglevm-core/util/assert.h"
#include "Zydis/Internal/FormatterBase.h"

namespace eagle::codec
{
    void setup_decoder()
    {
        ZydisDecoderInit(&zyids_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        ZydisFormatterInit(&zydis_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
    }

    reg get_bit_version(reg input_reg, const reg_size target_size)
    {
        const reg_class reg_class = get_class_from_size(target_size);
        return get_bit_version(input_reg, reg_class);
    }

    reg get_bit_version(reg input_reg, const reg_class target_size)
    {
        // unfortuantely this has to be manually resolved : (
        const reg_class source_class = get_reg_class(input_reg);

        const zydis_register zy_register = static_cast<zydis_register>(input_reg);
        const zydis_reg_class zy_reg_class = static_cast<zydis_reg_class>(target_size);

        uint8_t id = ZydisRegisterGetId(zy_register);
        if (source_class == gpr_8)
            if (id >= 4)
                id -= 4;

        if (target_size == gpr_8)
        {
            if (id >= 4)
                return static_cast<reg>(ZydisRegisterEncode(zy_reg_class, id + 4));
        }

        return static_cast<reg>(ZydisRegisterEncode(zy_reg_class, id));
    }

    reg get_bit_version(zydis_register input_reg, const reg_class target_size)
    {
        return get_bit_version(static_cast<reg>(input_reg), target_size);
    }

    reg_class get_max_size(reg input_reg)
    {
        const zydis_register zy_register = static_cast<zydis_register>(input_reg);
        const zydis_reg_class zy_reg_class = ZydisRegisterGetClass(zy_register);

        zydis_reg_class class_target;
        switch (zy_reg_class)
        {
            case ZYDIS_REGCLASS_GPR8:
            case ZYDIS_REGCLASS_GPR16:
            case ZYDIS_REGCLASS_GPR32:
            case ZYDIS_REGCLASS_GPR64:
                class_target = ZYDIS_REGCLASS_GPR64;
                break;
            case ZYDIS_REGCLASS_XMM:
            case ZYDIS_REGCLASS_YMM:
            case ZYDIS_REGCLASS_ZMM:
                class_target = ZYDIS_REGCLASS_ZMM;
                break;
            default:
                class_target = ZYDIS_REGCLASS_INVALID;
                break;
        }

        // this is really really bad, this should be a complete wrapper around zydis not just bound to restrictions of the virtualizer
        // will finish later and add limitations along with asserts.
        VM_ASSERT(class_target != ZYDIS_REGCLASS_INVALID, "Invalid register input size");
        return static_cast<reg_class>(class_target);
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

    reg_class get_class_from_size(const reg_size size)
    {
        switch (size)
        {
            case bit_64:
                return gpr_64;
            case bit_32:
                return gpr_32;
            case bit_16:
                return gpr_16;
            case bit_8:
                return gpr_8;
            case bit_512:
                return zmm_512;
            case bit_256:
                return ymm_256;
            case bit_128:
                return xmm_128;
            default:
            {
                VM_ASSERT("invalud reg_size for xmm class");
                return invalid;
            }
        }
    }

    reg_class get_reg_class(const reg reg)
    {
        const auto zy_register = static_cast<zydis_register>(reg);
        return get_reg_class(zy_register);
    }

    reg_class get_reg_class(const zydis_register reg)
    {
        return static_cast<reg_class>(ZydisRegisterGetClass(reg));
    }

    reg_size get_reg_size(const reg reg)
    {
        const reg_class reg_class = get_reg_class(reg);
        return get_reg_size(reg_class);
    }

    reg_size get_reg_size(const zydis_register reg)
    {
        const reg_class reg_class = get_reg_class(reg);
        return get_reg_size(reg_class);
    }

    reg_size get_reg_size(const reg_class reg)
    {
        switch (reg)
        {
            case gpr_64:
                return reg_size::bit_64;
            case gpr_32:
                return reg_size::bit_32;
            case gpr_16:
                return reg_size::bit_16;
            case gpr_8:
                return reg_size::bit_8;
            case mmx_64:
                return reg_size::bit_64;
            case xmm_128:
                return reg_size::bit_128;
            case ymm_256:
                return reg_size::bit_256;
            case zmm_512:
                return reg_size::bit_512;
            default:
                return reg_size::empty;
        }
    }

    char reg_size_to_string(const reg_class reg_size)
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

    enc::req create_encode_request(const mnemonic mnemonic)
    {
        enc::req req;
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

    void add_op(enc::req& req, const enc::op_imm imm)
    {
        const auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
        req.operands[op_index].imm = imm;
        req.operand_count++;
    }

    void add_op(enc::req& req, const enc::op_mem mem)
    {
        const auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_MEMORY;
        req.operands[op_index].mem = mem;
        req.operand_count++;
    }

    void add_op(enc::req& req, const enc::op_ptr ptr)
    {
        const auto op_index = req.operand_count;

        req.operands[op_index].type = ZYDIS_OPERAND_TYPE_POINTER;
        req.operands[op_index].ptr = ptr;
        req.operand_count++;
    }

    void add_op(enc::req& req, enc::op_reg reg)
    {
        const auto op_index = req.operand_count;

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

    const char* reg_to_string(reg reg)
    {
        return ZydisRegisterGetString(static_cast<ZydisRegister>(reg));
    }

    std::vector<uint8_t> compile(enc::req& request)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        const ZyanStatus result = ZydisEncoderEncodeInstruction(&request, instruction_data.data(), &encoded_length);
        if (!ZYAN_SUCCESS(result))
            __debugbreak();

        instruction_data.resize(encoded_length);
        return instruction_data;
    }

    std::vector<uint8_t> compile_absolute(enc::req& request, uint32_t address)
    {
        std::vector<uint8_t> instruction_data(ZYDIS_MAX_INSTRUCTION_LENGTH);
        ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;

        const ZyanStatus result = ZydisEncoderEncodeInstructionAbsolute(&request, instruction_data.data(),
            &encoded_length, address);
        if (!ZYAN_SUCCESS(result))
            __debugbreak();

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

            const ZyanStatus result = ZydisEncoderEncodeInstruction(&i, instruction_data.data(), &encoded_length);
            if (!ZYAN_SUCCESS(result))
                __debugbreak();

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

    std::vector<std::string> print_queue(const std::vector<enc::req>& queue,
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

    std::pair<uint64_t, uint8_t> calc_relative_rva(
        const dec::inst& instruction,
        const dec::operand* operands,
        const uint32_t rva,
        const int8_t operand)
    {
        uint64_t target_address = rva;
        if (operand < 0)
        {
            for (int i = 0; i < instruction.operand_count; i++)
            {
                const auto result = ZydisCalcAbsoluteAddress(&instruction, &operands[i], rva,
                    &target_address);
                if (result == ZYAN_STATUS_SUCCESS)
                    return { target_address, i };
            }
        }
        else
        {
            const auto result = ZydisCalcAbsoluteAddress(&instruction, &operands[operand],
                rva, &target_address);
            if (result == ZYAN_STATUS_SUCCESS)
                return { target_address, operand };
        }

        return { target_address, -1 };
    }

    std::pair<uint64_t, uint8_t> calc_relative_rva(const dec::inst_info& decode, const uint32_t rva, const int8_t operand)
    {
        const auto& [instruction, operands] = decode;

        uint64_t target_address = rva;
        if (operand < 0)
        {
            for (int i = 0; i < instruction.operand_count; i++)
            {
                auto result = ZydisCalcAbsoluteAddress(&instruction, &operands[i], rva, &target_address);
                if (result == ZYAN_STATUS_SUCCESS)
                    return { target_address, i };
            }
        }
        else
        {
            auto result = ZydisCalcAbsoluteAddress(&instruction, &operands[operand], rva, &target_address);
            if (result == ZYAN_STATUS_SUCCESS)
                return { target_address, operand };
        }

        return { target_address, -1 };
    }

    std::vector<dec::inst_info> get_instructions(void* data, size_t size)
    {
        std::vector<dec::inst_info> decode_data;

        ZyanUSize offset = 0;
        dec::inst_info decoded_instruction{ };

        while (ZYAN_SUCCESS(
            ZydisDecoderDecodeFull(&zyids_decoder, static_cast<char*>(data) + offset, size - offset
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
