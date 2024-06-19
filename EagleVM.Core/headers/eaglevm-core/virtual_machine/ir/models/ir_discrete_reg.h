#pragma once
#include <memory>
#include "eaglevm-core/util/assert.h"
#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir
{
    using discrete_store_ptr = std::shared_ptr<class discrete_store>;
    class discrete_store
    {
    public:
        explicit discrete_store(const ir_size size)
            : store_size(size), final_register(codec::reg::none), finalized(false)
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

        void finalize_register(const codec::reg destination)
        {
            VM_ASSERT(finalized == false, "warning: already finalized register");
            VM_ASSERT(destination != codec::reg::none, "warning: assigned empty register");

            final_register = destination;
            finalized = true;
        }

        [[nodiscard]] bool get_finalized() const
        {
            return finalized;
        }

        [[nodiscard]] codec::reg get_store_register(bool get_bit_64 = true) const
        {
            VM_ASSERT(finalized == true, "invalid retreival of unfinalized register");
            return final_register;
        }

    private:
        ir_size store_size;

        codec::reg final_register;
        bool finalized;
    };

}
