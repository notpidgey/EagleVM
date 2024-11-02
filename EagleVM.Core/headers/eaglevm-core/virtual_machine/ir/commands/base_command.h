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
    using x##_ptr = std::shared_ptr<x>

    class base_command : public std::enable_shared_from_this<base_command>
    {
    public:
        virtual ~base_command() = default;
        explicit base_command(command_type command, bool force_inline = false);

        command_type get_command_type() const;
        void set_inlined(bool inlined);
        bool is_inlined() const;

        virtual std::vector<discrete_store_ptr> get_use_stores();
        virtual bool is_similar(const std::shared_ptr<base_command>& other);

        static std::string cmd_type_to_string(command_type type);
        virtual std::string to_string();

        template<typename T>
        std::shared_ptr<T> get()
        {
            return std::static_pointer_cast<T>(shared_from_this());
        }

        uint32_t unique_id;
        std::string unique_id_string;

    protected:
        command_type type;
        bool force_inline;
    };

    SHARED_DEFINE(base_command);
    using ir_insts = std::vector<base_command_ptr>;
}
