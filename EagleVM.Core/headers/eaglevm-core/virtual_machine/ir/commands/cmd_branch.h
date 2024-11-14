#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/branch_command.h"

namespace eagle::ir
{
    enum class exit_location
    {
        none,
        block,
        rva
    };

    enum class exit_condition
    {
        none,

        // JO/JNO
        // OF = 1 / OF = 0
        jo,

        // JS/JNS
        // SF = 1 / SF = 0
        js,

        // JE/JNE
        // ZF = 1 / ZF = 0
        je,

        // JC/JNC
        // CF = 1 / CF = 0
        jb,

        // JBE/JNBE
        // CF = 1 ZF = 1 / CF = 0 ZF = 0
        jbe,

        // JL/JNL
        // SF <> OF / SF = OF
        jl,

        // JLE/JNLE
        // ZF = 1 SF <> OF / ZF = 0 SF = OF
        jle,

        // JP/JPO
        // PF = 1 / PF = 0
        jp,

        // JCX/ECX/RC
        // CX = 0 / ECX = 0 /
        jcxz,
        jecxz,
        jrcxz,

        jmp
    };

    class cmd_branch final : public branch_command, public base_command
    {
    public:
        explicit cmd_branch(const ir_exit_result& result_fallthrough);
        cmd_branch(const ir_exit_result& result_fallthrough, const ir_exit_result& result_conditional, exit_condition condition,
            bool invert_condition);

        [[nodiscard]] exit_condition get_condition() const;
        ir_exit_result& get_condition_default();
        ir_exit_result& get_condition_special();

        void set_virtual(bool is_virtual);
        bool is_virtual() const;
        bool is_inverted() const;

        std::string to_string() override;
        bool is_similar(const std::shared_ptr<base_command>& other) override;
        BASE_COMMAND_CLONE(cmd_branch)

    private:
        exit_condition condition;
        bool invert_condition;

        bool virtual_branch;
    };

    SHARED_DEFINE(cmd_branch);
}
