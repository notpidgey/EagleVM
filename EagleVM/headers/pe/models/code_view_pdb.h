#pragma once
#include <cstdint>

struct code_view_pdb
{
    char signature[4];
    char guid[16];
    uint32_t age;
};

