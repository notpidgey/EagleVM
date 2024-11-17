#pragma once
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "eaglevm-core/util/random.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_type.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

#include "eaglevm-core/util/assert.h"

namespace eagle::ir
{
#define SHARED_DEFINE(x) \
    using x##_ptr = std::shared_ptr<x>;

#define BASE_COMMAND_CLONE(x) \
    std::shared_ptr<base_command> clone() override { return std::make_shared< x >(*this); }

    class base_command : public std::enable_shared_from_this<base_command>
    {
    public:
        virtual ~base_command() = default;
        explicit base_command(command_type command, bool force_inline = false);

        command_type get_command_type() const;
        void set_inlined(bool inlined);
        bool is_inlined() const;

        template <typename T>
        std::shared_ptr<T> get()
        {
            return std::static_pointer_cast<T>(shared_from_this());
        }

        virtual bool is_similar(const std::shared_ptr<base_command>& other);
        virtual std::string to_string();
        virtual std::shared_ptr<base_command> clone() = 0;

        static std::string cmd_type_to_string(command_type type);

        uint32_t unique_id;
        std::string unique_id_string;

    protected:
        command_type type;
        bool force_inline;
    };

    SHARED_DEFINE(base_command);
    using ir_insts = std::vector<base_command_ptr>;
}
