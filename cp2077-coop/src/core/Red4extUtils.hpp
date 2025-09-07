#pragma once

#include <RED4ext/RED4ext.hpp>
#include <iostream>

// Wraps RED4ext::ExecuteFunction and logs failures.
// Usage:
//     RED4EXT_EXECUTE("Class", "Func", outPtr, args...);
#define RED4EXT_EXECUTE(aCls, aFunc, aRet, ...) \
    do \
    { \
        if (!RED4ext::ExecuteFunction(aCls, aFunc, aRet, ##__VA_ARGS__)) \
        { \
            std::cerr << "[RED4ext] call failed: " << aCls << "::" << aFunc << std::endl; \
        } \
    } while (0)

