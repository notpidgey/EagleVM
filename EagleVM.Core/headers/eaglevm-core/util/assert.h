#pragma once
#include <cassert>

#ifndef NDEBUG
    #define VM_ASSERT(condition, message) assert((condition) && (message))
    #define VM_ASSERT(condition) assert((condition))
#else
    #define VM_ASSERT(condition, message) do { } while (false)
#endif
