#include "eaglevm-core/virtual_machine/ir/x86/handlers/jcc.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/dynamic_encoder/encoder.h"
#include "eaglevm-core/virtual_machine/ir/x86/util.h"
#include "eaglevm-core/virtual_machine/ir/ir_translator.h"


#define HS(x) hash_string::hash(x)

namespace eagle::ir
{
    namespace handler
    {
        jcc::jcc()
        {
            build_options = {
                { { ir_size::bit_32 }, "jmp rel32" },

                { { ir_size::bit_32 }, "jo rel32" },
                { { ir_size::bit_32 }, "jno rel32" },

                { { ir_size::bit_32 }, "js rel32" },
                { { ir_size::bit_32 }, "jns rel32" },

                { { ir_size::bit_32 }, "je rel32" },
                { { ir_size::bit_32 }, "jne rel32" },

                { { ir_size::bit_32 }, "jc rel32" },
                { { ir_size::bit_32 }, "jnc rel32" },

                { { ir_size::bit_32 }, "jl rel32" },
                { { ir_size::bit_32 }, "jnl rel32" },

                { { ir_size::bit_32 }, "jle rel32" },
                { { ir_size::bit_32 }, "jnle rel32" },

                { { ir_size::bit_32 }, "jp rel32" },
                { { ir_size::bit_32 }, "jpo rel32" },

                { { ir_size::bit_32 }, "jcxz rel32" },
                { { ir_size::bit_32 }, "jecxz rel32" },
            };
        }

        ir_insts jcc::gen_handler(const uint64_t target_handler_id)
        {
            // method inspired by vmprotect 2
            // https://blog.back.engineering/21/06/2021/#vmemu-virtual-branching

            switch (const exit_condition condition = static_cast<exit_condition>(target_handler_id))
            {
                case exit_condition::jo:
                case exit_condition::js:
                case exit_condition::je:
                case exit_condition::jb:
                case exit_condition::jp:
                    write_condition_jump(get_flag_for_condition(condition));
                    break;
                case exit_condition::jbe:
                    write_bitwise_condition(codec::m_and, ZYDIS_CPUFLAG_CF, ZYDIS_CPUFLAG_ZF);
                    break;
                case exit_condition::jl:
                    write_bitwise_condition(codec::m_xor, ZYDIS_CPUFLAG_SF, ZYDIS_CPUFLAG_OF);
                    break;
                case exit_condition::jle:
                    write_jle();
                    break;
                case exit_condition::jcxz:
                case exit_condition::jecxz:
                case exit_condition::jrcxz:
                    write_check_register(get_register_for_condition(condition));
                    break;
                default:
                    VM_ASSERT("invalid jump condition");
                    break;
            }

            //                 std::make_shared<cmd_jmp>()

            return commands;
        }

        void jcc::write_condition_jump(const uint64_t flag_mask) const
        {
            write_flag_operation([flag_mask]
            {
                std::vector<base_command_ptr> command_vec;
                command_vec += load_isolated_flag(flag_mask);

                return command_vec;
            });
        }

        void jcc::write_bitwise_condition(codec::mnemonic bitwise, uint64_t flag_mask_one, uint64_t flag_mask_two) const
        {
            write_flag_operation([flag_mask_one, flag_mask_two, bitwise]
            {
                std::vector<base_command_ptr> command_vec;
                command_vec += load_isolated_flag(flag_mask_one);
                command_vec += load_isolated_flag(flag_mask_two);
                command_vec.push_back(std::make_shared<cmd_handler_call>(bitwise, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));

                return command_vec;
            });
        }

        void jcc::write_check_register(codec::reg reg) const
        {
            auto target_size = bits_to_ir_size(get_reg_size(reg));
            write_flag_operation([reg, target_size]
            {
                return std::vector<base_command_ptr>{
                    std::make_shared<cmd_context_load>(reg),
                    std::make_shared<cmd_push>(0, target_size),
                    std::make_shared<cmd_handler_call>(codec::m_cmp, handler_sig{ target_size, target_size })
                    // TODO: handle flags result
                };
            });
        }

        void jcc::write_jle() const
        {
            write_flag_operation([]
            {
                std::vector<base_command_ptr> command_vec;
                command_vec += load_isolated_flag(ZYDIS_CPUFLAG_SF);
                command_vec += load_isolated_flag(ZYDIS_CPUFLAG_OF);
                command_vec.push_back(std::make_shared<cmd_handler_call>(codec::m_xor, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));

                command_vec += load_isolated_flag(ZYDIS_CPUFLAG_ZF);
                command_vec.push_back(std::make_shared<cmd_handler_call>(codec::m_or, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));

                return command_vec;
            });
        }

        void jcc::write_flag_operation(const std::function<std::vector<base_command_ptr>()>& operation_generator) const
        {
            std::vector<base_command_ptr> commands;

            auto operation_commands = operation_generator();
            commands.insert(commands.end(), operation_commands.begin(), operation_commands.end());

            commands.insert(commands.end(), {
                std::make_shared<cmd_handler_call>(codec::m_add, handler_sig{ ir_size::bit_64, ir_size::bit_64 }),
                std::make_shared<cmd_mem_read>(ir_size::bit_64),
            });
        }

        std::vector<base_command_ptr> jcc::load_isolated_flag(const uint64_t flag_mask)
        {
            long index;
            _BitScanForward(reinterpret_cast<unsigned long*>(&index), flag_mask);

            const auto shift_cmd = index > 3
                                       ? std::make_shared<cmd_handler_call>(codec::m_shl, handler_sig{ ir_size::bit_64, ir_size::bit_64 })
                                       : std::make_shared<cmd_handler_call>(codec::m_shr, handler_sig{ ir_size::bit_64, ir_size::bit_64 });

            return std::vector<base_command_ptr>{
                std::make_shared<cmd_rflags_load>(),

                // mask the flag
                std::make_shared<cmd_push>(flag_mask, ir_size::bit_64),
                std::make_shared<cmd_handler_call>(codec::m_and, handler_sig{ ir_size::bit_64, ir_size::bit_64 }),

                // shift from original flag to bit 3
                std::make_shared<cmd_push>(index, ir_size::bit_64),
                shift_cmd
            };
        }

        uint64_t jcc::get_flag_for_condition(const exit_condition condition)
        {
            switch (condition)
            {
                case exit_condition::jo: return ZYDIS_CPUFLAG_OF;
                case exit_condition::js: return ZYDIS_CPUFLAG_SF;
                case exit_condition::je: return ZYDIS_CPUFLAG_ZF;
                case exit_condition::jb: return ZYDIS_CPUFLAG_CF;
                case exit_condition::jp: return ZYDIS_CPUFLAG_PF;
                default: VM_ASSERT("Invalid condition for get_flag_for_condition");
            }
            return 0;
        }

        codec::reg jcc::get_register_for_condition(const exit_condition condition)
        {
            switch (condition)
            {
                case exit_condition::jcxz: return codec::reg::cx;
                case exit_condition::jecxz: return codec::reg::ecx;
                case exit_condition::jrcxz: return codec::reg::rcx;
                default: VM_ASSERT("Invalid condition for get_register_for_condition");
            }
            return codec::reg::none;
        }
    }

#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

    namespace lifter
    {
        bool jcc::translate_to_il(const uint64_t original_rva, const x86_cpu_flag flags)
        {
            // jcc is one of those odd commands that i haven't really figured out how to properly handle
            // i wanted to initially dump all the branching logic right into the block but that doesnt make much sense
            // because then you would not be able to analyze the IR and figure out how the block branches
            // the simplest way is to insert a branch cmd, and have the virtual machine implementer call the JCC handler
            // depending on the implementer, they could also decide to inline all the JCC handlers so that it results in the same result

            const auto [condition, inverted] = get_exit_condition(static_cast<codec::mnemonic>(inst.mnemonic));

            branch_info info = translator->get_branch_info(original_rva);
            if (info.exit_condition == exit_condition::jmp)
                block->push_back(std::make_shared<cmd_branch>(info.fallthrough_branch.value()));
            else
                block->push_back(std::make_shared<cmd_branch>(*info.fallthrough_branch, *info.conditional_branch, condition,
                    info.inverted_condition));

            return true;
        }

        std::pair<exit_condition, bool> jcc::get_exit_condition(const codec::mnemonic mnemonic)
        {
            switch (mnemonic)
            {
                // Overflow flag (OF)
                case codec::m_jo: return { exit_condition::jo, false };
                case codec::m_jno: return { exit_condition::jo, true };

                // Sign flag (SF)
                case codec::m_js: return { exit_condition::js, false };
                case codec::m_jns: return { exit_condition::js, true };

                // Zero flag (ZF)
                case codec::m_jz: return { exit_condition::je, false };
                case codec::m_jnz: return { exit_condition::je, true };

                // Carry flag (CF)
                case codec::m_jb: return { exit_condition::jb, false };
                case codec::m_jnb: return { exit_condition::jb, true };

                // Carry or Zero flag (CF or ZF)
                case codec::m_jbe: return { exit_condition::jbe, false };
                case codec::m_jnbe: return { exit_condition::jbe, true };

                // Sign flag not equal to Overflow flag (SF != OF)
                case codec::m_jl: return { exit_condition::jl, false };
                case codec::m_jnl: return { exit_condition::jl, true };

                // Zero flag or Sign flag not equal to Overflow flag (ZF or SF != OF)
                case codec::m_jle: return { exit_condition::jle, false };
                case codec::m_jnle: return { exit_condition::jle, true };

                // Parity flag (PF)
                case codec::m_jp: return { exit_condition::jp, false };
                case codec::m_jnp: return { exit_condition::jp, true };

                // CX/ECX/RCX register is zero
                case codec::m_jcxz: return { exit_condition::jcxz, false };
                case codec::m_jecxz: return { exit_condition::jecxz, false };
                case codec::m_jrcxz: return { exit_condition::jrcxz, false };

                // Unconditional jump
                case codec::m_jmp: return { exit_condition::jmp, false };
                default:
                {
                    VM_ASSERT("invalid conditional jump reached");
                    return { exit_condition::none, false };
                }
            }
        }
    }
}