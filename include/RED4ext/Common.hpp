#pragma once

#include <cstddef>

#ifdef RED4EXT_STATIC_LIB
#undef RED4EXT_HEADER_ONLY
#define RED4EXT_INLINE
#else
#define RED4EXT_HEADER_ONLY
#define RED4EXT_INLINE inline
#endif

#ifndef RED4EXT_ASSERT_ESCAPE
#define RED4EXT_ASSERT_ESCAPE(...) __VA_ARGS__
#endif

#ifndef RED4EXT_ASSERT_SIZE
#define RED4EXT_ASSERT_SIZE(cls, size)                                                                                 \
    static_assert(sizeof(cls) == size, #cls " size does not match the expected size (" #size ") ")
#endif

#ifndef RED4EXT_ASSERT_OFFSET
#  if defined(__has_builtin)
#    if __has_builtin(__builtin_offsetof)
#      define RED4EXT_ASSERT_OFFSET(cls, mbr, offset) \
        static_assert(__builtin_offsetof(cls, mbr) == offset, #cls "::" #mbr " is not on the expected offset (" #offset ")")
#    elif defined(__builtin_offsetof)
#      define RED4EXT_ASSERT_OFFSET(cls, mbr, offset) \
        static_assert(__builtin_offsetof(cls, mbr) == offset, #cls "::" #mbr " is not on the expected offset (" #offset ")")
#    else
#      define RED4EXT_ASSERT_OFFSET(cls, mbr, offset) \
        static_assert(offsetof(cls, mbr) == offset, #cls "::" #mbr " is not on the expected offset (" #offset ")")
#    endif
#  elif defined(__builtin_offsetof)
#    define RED4EXT_ASSERT_OFFSET(cls, mbr, offset) \
      static_assert(__builtin_offsetof(cls, mbr) == offset, #cls "::" #mbr " is not on the expected offset (" #offset ")")
#  else
#    define RED4EXT_ASSERT_OFFSET(cls, mbr, offset) \
      static_assert(offsetof(cls, mbr) == offset, #cls "::" #mbr " is not on the expected offset (" #offset ")")
#  endif
#endif

#define REL_OFFS_ASSERT(strct, member, off) \
    static_assert(offsetof(strct, member) == off, #member " offset mismatch")

/**
 * @brief This macro is used to avoid compiler warnings about unreferenced / used parameter.
 */
#ifndef RED4EXT_UNUSED_PARAMETER
#define RED4EXT_UNUSED_PARAMETER(param) (param)
#endif

#ifndef RED4EXT_DECLARE_TYPE
#define RED4EXT_DECLARE_TYPE(type, name)                                                                               \
    const type* const_##name;                                                                                          \
    type* name;
#endif

#ifndef RED4EXT_C_EXPORT
#define RED4EXT_C_EXPORT extern "C" __declspec(dllexport)
#endif

#ifndef RED4EXT_CALL
#define RED4EXT_CALL __fastcall
#endif

// Basic offset examples from RTTI (only when core types are available)
#ifdef RED4EXT_OFFSETS_ENABLED
REL_OFFS_ASSERT(::RED4ext::CName, hash, 0x0);
REL_OFFS_ASSERT(::RED4ext::Vector4, X, 0x0);
REL_OFFS_ASSERT(::RED4ext::Vector4, Y, 0x4);
REL_OFFS_ASSERT(::RED4ext::Vector4, Z, 0x8);
REL_OFFS_ASSERT(::RED4ext::Vector4, W, 0xC);
REL_OFFS_ASSERT(::RED4ext::Quaternion, i, 0x0);
REL_OFFS_ASSERT(::RED4ext::Quaternion, j, 0x4);
REL_OFFS_ASSERT(::RED4ext::Quaternion, k, 0x8);
REL_OFFS_ASSERT(::RED4ext::Quaternion, r, 0xC);
#endif
