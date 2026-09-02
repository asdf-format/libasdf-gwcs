/**
 * Internal utility macros for libasdf-gwcs
 */
#pragma once

#include <assert.h>
#include <string.h>

#include <asdf/util.h>


/**
 * Return a heap-allocated canonical tag string (always "tag:..." form)
 * Prepends "tag:" if absent.  Caller must free.  Returns NULL on OOM.
 *
 * TODO: This duplicates logic in libasdf; add to its public API someday.
 */
ASDF_LOCAL char *tag_canonicalize(const char *tag);


/**
 * Write into dest the type name a schema tag denotes
 *
 * The type name is the last path element of the tag with its version suffix
 * removed, so "tag:stsci.edu:asdf/transform/affine-1.5.0" yields "affine".
 * dest is always NUL-terminated; a name longer than size - 1 is truncated.
 */
ASDF_LOCAL void tag_type_name(char *dest, size_t size, const char *tag);


#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif


#if defined(__GNUC__) || defined(__clang__)
#define UNUSED(x) x __attribute__((unused))
#else
#define UNUSED(x) (void)(x)
#endif


#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE() \
    assert(false && "unreachable"); \
    __builtin_unreachable()
#else
#define UNREACHABLE() assert(false && "unreachable")
#endif


/* Cast memset to volatile to prevent opt-away */
#define ZERO_MEMORY(ptr, size) \
    do { \
        void *volatile _volatile_ptr = (ptr); \
        memset(_volatile_ptr, 0, (size)); \
    } while (0)
