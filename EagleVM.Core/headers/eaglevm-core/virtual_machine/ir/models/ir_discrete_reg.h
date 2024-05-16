#pragma once
#include <memory>
#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir
{
    using discrete_store_ptr = std::shared_ptr<class discrete_store>;
    class discrete_store
    {
    public:
        explicit discrete_store(const ir_size size)
            : store_size(size)
        {

        }

        static discrete_store_ptr create(ir_size size)
        {
            return std::make_shared<discrete_store>(size);
        }

        [[nodiscard]] ir_size get_store_size() const
        {
            return store_size;
        }

    private:
        ir_size store_size;
    };

}
