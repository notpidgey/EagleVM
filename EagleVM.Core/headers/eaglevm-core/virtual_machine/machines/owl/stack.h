#pragma once
#include "eaglevm-core/virtual_machine/machines/register_context.h"

namespace eagle::virt::owl
{
    class stack_loader
    {
    public:
        explicit stack_loader(codec::encoder::encode_builder& builder, const std::function<codec::reg(codec::reg_class)>& alloc)
            : builder(builder), alloc_reg(alloc)
        {
        }

        void push_ymm_lane(codec::reg ymm_input_reg, uint16_t input_lane);
        void push_gpr(codec::reg gpr_input_reg);

        void pop_to_ymm(codec::reg ymm_out, uint16_t qword_lane_out, codec::reg_size size, bool peek = false);
        std::pair<codec::reg, codec::reg> pop_64(bool peek = false);

    private:
        // register_manager_ptr regs;
        codec::encoder::encode_builder& builder;
        std::function<codec::reg(codec::reg_size)> alloc_reg;

        void gen_vsp_modification(codec::encoder::encode_builder& out, bool sub);
        void sub_vsp(codec::encoder::encode_builder& out);
        void add_vsp(codec::encoder::encode_builder& out);
    };
}
