#pragma once
#include "eaglevm-core/pe/pe_generator.h"
#include "eaglevm-core/compiler/section_manager.h"

namespace eagle::pe
{
    class pe_packer
    {
    public:
        explicit pe_packer(pe_generator* generator)
            : generator(generator) {}

        void set_overlay(bool overlay);
        static std::pair<uint32_t, uint32_t> insert_pdb(codec::encoded_vec& encoded_vec);

        asmb::section_manager create_section() const;

    private:
        pe_generator* generator;

        bool text_overlay;
        bool pdb_rewrite;
        // other future features;
    };
}