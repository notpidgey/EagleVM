#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

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

    using vmexit_rva = uint64_t;
    using block_ptr = std::shared_ptr<class block_ir>;
    using il_exit_result = std::variant<vmexit_rva, block_ptr>;

    class cmd_branch final : public base_command
    {
    public:
        explicit cmd_branch(const il_exit_result& result_info);
        cmd_branch(const std::vector<il_exit_result>& result_info, exit_condition condition, bool invert);

        [[nodiscard]] exit_condition get_condition() const;
        il_exit_result& get_condition_default();
        il_exit_result& get_condition_special();
        bool get_inverted();

        bool branch_visits(vmexit_rva rva);
        bool branch_visits(const block_ptr& block);

        void rewrite_branch(const il_exit_result& search, const il_exit_result& target);
        bool is_similar(const std::shared_ptr<base_command>& other) override;

    private:
        std::vector<il_exit_result> info;
        exit_condition condition;

        bool inverted;
    };
}
