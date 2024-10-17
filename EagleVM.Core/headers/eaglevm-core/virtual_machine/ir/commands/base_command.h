#pragma once
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "eaglevm-core/util/random.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_type.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"

#include "eaglevm-core/util/assert.h"

namespace eagle::ir
{
#define SHARED_DEFINE(x) \
    class x; \
    using x##_ptr = std::shared_ptr<x>

    SHARED_DEFINE(cmd_context_load);
    SHARED_DEFINE(cmd_context_store);
    SHARED_DEFINE(cmd_vm_enter);
    SHARED_DEFINE(cmd_vm_exit);
    SHARED_DEFINE(cmd_handler_call);
    SHARED_DEFINE(cmd_mem_read);
    SHARED_DEFINE(cmd_mem_write);
    SHARED_DEFINE(cmd_pop);
    SHARED_DEFINE(cmd_push);
    SHARED_DEFINE(cmd_context_rflags_load);
    SHARED_DEFINE(cmd_context_rflags_store);
    SHARED_DEFINE(cmd_sx);
    SHARED_DEFINE(cmd_x86_dynamic);
    SHARED_DEFINE(cmd_x86_exec);
    SHARED_DEFINE(cmd_branch);

    class base_command : public std::enable_shared_from_this<base_command>
    {
    public:
        virtual ~base_command() = default;
        explicit base_command(const command_type command, const bool force_inline = false)
            : type(command), force_inline(force_inline)
        {
            static uint32_t id = 0;

            unique_id = id;
            unique_id_string = command_to_string(command) + ": " + std::to_string(id++);
        }

        command_type get_command_type() const;
        bool is_inlined();

        virtual bool is_similar(const std::shared_ptr<base_command>& other)
        {
            return other->get_command_type() == get_command_type();
        }

        uint32_t unique_id;
        std::string unique_id_string;

    protected:
        command_type type;

        bool force_inline;

    private:
        static std::string command_to_string(command_type type);
    };

    SHARED_DEFINE(base_command);
    using ir_insts = std::vector<base_command_ptr>;
}
