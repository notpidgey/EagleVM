#pragma once
#include <cassert>

// credit: https://github.dev/x64dbg/x64dbg
#pragma once
#include <cassert>

#ifdef _DEBUG
    #define __DBG_ARGUMENT_EXPAND(x) x
    #define __DBG_ARGUMENT_COUNT(...) \
    __DBG_ARGUMENT_EXPAND(__DBG_ARGUMENT_EXTRACT(__VA_ARGS__, 2, 1, 0))
    #define __DBG_ARGUMENT_EXTRACT(a1, a2, N, ...) N

    #define __DBG_MACRO_DISPATCH(function, ...) \
    __DBG_MACRO_SELECT(function, __DBG_ARGUMENT_COUNT(__VA_ARGS__))
    #define __DBG_MACRO_SELECT(function, argc) \
    __DBG_MACRO_CONCAT(function, argc)
    #define __DBG_MACRO_CONCAT(a, b) a##b

    #define VM_ASSERT(...) \
        __DBG_MACRO_DISPATCH(VM_ASSERT, __VA_ARGS__)(__VA_ARGS__)
    #define VM_ASSERT1(message) assert((false) && (message))  // Auto-assert true with a single message argument
    #define VM_ASSERT2(expr, message) assert((expr) && (message))
#else
    #define VM_ASSERT(...) do { } while (false)
#endif
