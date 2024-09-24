#include "eaglevm-core/virtual_machine/ir/x86/handlers/jcc.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#define HS(x) hash_string::hash(x)

namespace eagle::ir
{
    namespace handler
    {
        jcc::jcc()
        {
            build_options = {
                { { ir_size::bit_32 }, "jo rel32" },
                { { ir_size::bit_32 }, "jno rel32" },

                { { ir_size::bit_32 }, "js rel32" },
                { { ir_size::bit_32 }, "jns rel32" },

                { { ir_size::bit_32 }, "je rel32" },
                { { ir_size::bit_32 }, "jne rel32" },

                { { ir_size::bit_32 }, "jc rel32" },
                { { ir_size::bit_32 }, "jnc rel32" },

                { { ir_size::bit_32 }, "jb rel32" },
                { { ir_size::bit_32 }, "jnb rel32" },

                { { ir_size::bit_32 }, "jl rel32" },
                { { ir_size::bit_32 }, "jnl rel32" },

                { { ir_size::bit_32 }, "jle rel32" },
                { { ir_size::bit_32 }, "jnle rel32" },

                { { ir_size::bit_32 }, "jno rel32" },

                { { ir_size::bit_32 }, "jcxz rel32" },
                { { ir_size::bit_32 }, "jecxz rel32" },
            };
        }

        ir_insts jcc::gen_handler(const uint64_t target_handler_id)
        {
            // method inspired by vmprotect 2
            // https://blog.back.engineering/21/06/2021/#vmemu-virtual-branching

            const discrete_store_ptr branch_one = discrete_store::create(ir_size::bit_64);
            const discrete_store_ptr branch_two = discrete_store::create(ir_size::bit_64);
            const discrete_store_ptr flags = discrete_store::create(ir_size::bit_64);

            auto create_default_flag = [&](uint32_t flag) -> ir_insts
            {
                unsigned long bit_idx = 0;
                _BitScanForward(&bit_idx, flag);

                uint8_t shift_count = 0;
                codec::mnemonic shift_dir = codec::m_shr;
                if (bit_idx < 8)
                {
                    shift_count = 8 - bit_idx;
                    shift_dir = codec::m_shl;
                }
                else
                {
                    shift_count = bit_idx - 8;
                }

                return {
                    std::make_shared<cmd_rflags_push>(),

                    std::make_shared<cmd_rflags_load>(),
                    std::make_shared<cmd_pop>(flags, ir_size::bit_64),

                    // get the relevant flag
                    std::make_shared<cmd_x86_dynamic>(codec::m_and, flags, flag),
                    std::make_shared<cmd_x86_dynamic>(shift_dir, flags, shift_count),

                    std::make_shared<cmd_x86_dynamic>(codec::m_mov, flags, [vsp + flags * 1]),
                    std::make_shared<cmd_x86_dynamic>(codec::m_add, reg_vm::vsp, 0x10),

                    std::make_shared<cmd_rflags_pop>(),
                    std::make_shared<cmd_x86_dynamic>(codec::m_jmp, flags)
                };
            };

            switch (target_handler_id)
            {
                // Overflow and no overflow cases
                case HS("jo rel32"):
                case HS("jno rel32"):
                    return create_default_flag(ZYDIS_CPUFLAG_OF);

                // Sign and no sign cases
                case HS("js rel32"):
                case HS("jns rel32"):
                    return create_default_flag(ZYDIS_CPUFLAG_SF);

                // Equal (zero) and not equal (not zero) cases
                case HS("je rel32"):
                case HS("jne rel32"):
                return create_default_flag(ZYDIS_CPUFLAG_ZF);

                // Carry and no carry cases
                case HS("jc rel32"):
                case HS("jnc rel32"):
                return create_default_flag(ZYDIS_CPUFLAG_CF);

                // Below and not below (above or equal) cases
                case HS("jb rel32"):
                case HS("jnb rel32"):
                    return ...;

                // Less and not less (greater or equal) cases
                case HS("jl rel32"):
                case HS("jnl rel32"):
                    return ...;

                // Less or equal and not less or equal (greater) cases
                case HS("jle rel32"):
                case HS("jnle rel32"):
                    // Add specific handler code if needed
                    break;

                // Jump if CX/ECX is zero cases
                case HS("jcxz rel32"):
                case HS("jecxz rel32"):
                    // Add specific handler code if needed
                    break;

                default:
                    return base_handler_gen::gen_handler(target_handler_id);
            }

            // Return handler or perform additional logic here if needed
            return ir_insts();
        }
    }

    namespace lifter
    {
        bool jcc::translate_to_il(const uint64_t original_rva, const x86_cpu_flag flags)
        {
            const exit_condition condition = get_exit_condition(static_cast<codec::mnemonic>(inst.mnemonic));
            block->push_back(std::make_shared<cmd_handler_call>(inst.mnemonic, ""));

            return true;
        }

        exit_condition jcc::get_exit_condition(const codec::mnemonic mnemonic)
        {
            switch (mnemonic)
            {
                case codec::m_jb:
                    return exit_condition::jb;
                case codec::m_jbe:
                    return exit_condition::jbe;
                case codec::m_jcxz:
                    return exit_condition::jcxz;
                case codec::m_jecxz:
                    return exit_condition::jecxz;
                case codec::m_jknzd:
                    return exit_condition::jknzd;
                case codec::m_jkzd:
                    return exit_condition::jkzd;
                case codec::m_jl:
                    return exit_condition::jl;
                case codec::m_jle:
                    return exit_condition::jle;
                case codec::m_jmp:
                    return exit_condition::jmp;
                case codec::m_jnb:
                    return exit_condition::jnb;
                case codec::m_jnbe:
                    return exit_condition::jnbe;
                case codec::m_jnl:
                    return exit_condition::jnl;
                case codec::m_jnle:
                    return exit_condition::jnle;
                case codec::m_jno:
                    return exit_condition::jno;
                case codec::m_jnp:
                    return exit_condition::jnp;
                case codec::m_jns:
                    return exit_condition::jns;
                case codec::m_jnz:
                    return exit_condition::jnz;
                case codec::m_jo:
                    return exit_condition::jo;
                case codec::m_jp:
                    return exit_condition::jp;
                case codec::m_jrcxz:
                    return exit_condition::jrcxz;
                case codec::m_js:
                    return exit_condition::js;
                case codec::m_jz:
                    return exit_condition::jz;
                default:
                {
                    VM_ASSERT("invalid conditional jump reached");
                    return exit_condition::none;
                }
            }
        }
    }
}
