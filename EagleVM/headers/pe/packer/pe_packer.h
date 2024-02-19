#pragma once
#include "pe/pe_generator.h"
#include "util/section/section_manager.h"

class pe_packer
{
public:
    explicit pe_packer(pe_generator* generator)
        : generator(generator) {}

    void set_overlay(bool overlay);

    section_manager create_section();

private:
    pe_generator* generator;

    bool text_overlay;
    // other future features;
};