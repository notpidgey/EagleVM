#include "eaglevm-core/virtual_machine/ir/x86/handlers/jcc.h"

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

        ir_insts jcc::gen_handler(handler_sig signature)
        {
            switch(signature.)
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
