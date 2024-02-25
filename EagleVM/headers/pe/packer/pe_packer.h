#pragma once
#include "pe/pe_generator.h"
#include "util/section/section_manager.h"

class pe_packer
{
public:
    explicit pe_packer(pe_generator* generator)
        : generator(generator) {}

    void set_overlay(bool overlay);
    static std::pair<uint32_t, uint32_t> insert_pdb(encoded_vec& encoded_vec);

    section_manager create_section();

private:
    pe_generator* generator;

    bool text_overlay;
    bool pdb_rewrite;
    // other future features;
};