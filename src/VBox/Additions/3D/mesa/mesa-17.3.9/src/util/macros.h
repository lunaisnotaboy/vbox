/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UTIL_MACROS_H
#define UTIL_MACROS_H

#include <assert.h>

#include "c99_compat.h"

/* Compute the size of an array */
#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* For compatibility with Clang's __has_builtin() */
#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

/**
 * __builtin_expect macros
 */
#if !defined(HAVE___BUILTIN_EXPECT)
#  define __builtin_expect(x, y) (x)
#endif

#ifndef likely
#  ifdef HAVE___BUILTIN_EXPECT
#    define likely(x)   __builtin_expect(!!(x), 1)
#    define unlikely(x) __builtin_expect(!!(x), 0)
#  else
#    define likely(x)   (x)
#    define unlikely(x) (x)
#  endif
#endif


/**
 * Static (compile-time) assertion.
 * Basically, use COND to dimension an array.  If COND is false/zero the
 * array size will be -1 and we'll get a compilation error.
 */
#define STATIC_ASSERT(COND) \
   do { \
      (void) sizeof(char [1 - 2*!(COND)]); \
   } while (0)


/**
 * Unreachable macro. Useful for suppressing "control reaches end of non-void
 * function" warnings.
 */
#ifdef HAVE___BUILTIN_UNREACHABLE
#define unreachable(str)    \
do {                        \
   assert(!str);            \
   __builtin_unreachable(); \
} while (0)
#elif defined (_MSC_VER)
#define unreachable(str)    \
do {                        \
   assert(!str);            \
   __assume(0);             \
} while (0)
#else
#define unreachable(str) assert(!str)
#endif

/**
 * Assume macro. Useful for expressing our assumptions to the compiler,
 * typically for purposes of silencing warnings.
 */
#if __has_builtin(__builtin_assume)
#define assume(expr)       \
do {                       \
   assert(expr);           \
   __builtin_assume(expr); \
} while (0)
#elif defined HAVE___BUILTIN_UNREACHABLE
#define assume(expr) ((expr) ? ((void) 0) \
                             : (assert(!"assumption failed"), \
                                __builtin_unreachable()))
#elif defined (_MSC_VER)
#define assume(expr) __assume(expr)
#else
#define assume(expr) assert(expr)
#endif

/* Attribute const is used for functions that have no effects other than their
 * return value, and only rely on the argument values to compute the return
 * value.  As a result, calls to it can be CSEed.  Note that using memory
 * pointed to by the arguments is not allowed for const functions.
 */
#ifdef HAVE_FUNC_ATTRIBUTE_CONST
#define ATTRIBUTE_CONST __attribute__((__const__))
#else
#define ATTRIBUTE_CONST
#endif

#ifdef HAVE_FUNC_ATTRIBUTE_FLATTEN
#define FLATTEN __attribute__((__flatten__))
#else
#define FLATTEN
#endif

#ifdef HAVE_FUNC_ATTRIBUTE_FORMAT
#define PRINTFLIKE(f, a) __attribute__ ((format(__printf__, f, a)))
#else
#define PRINTFLIKE(f, a)
#endif

#ifdef HAVE_FUNC_ATTRIBUTE_MALLOC
#define MALLOCLIKE __attribute__((__malloc__))
#else
#define MALLOCLIKE
#endif

/* Forced function inlining */
#ifndef ALWAYS_INLINE
#  if defined(__GNUC__) || defined(__clang__)
#    define ALWAYS_INLINE inline __attribute__((always_inline))
#  elif defined(_MSC_VER)
#    define ALWAYS_INLINE __forceinline
#  else
#    define ALWAYS_INLINE inline
#  endif
#endif

/* Used to optionally mark structures with misaligned elements or size as
 * packed, to trade off performance for space.
 */
#ifdef HAVE_FUNC_ATTRIBUTE_PACKED
#define PACKED __attribute__((__packed__))
#else
#define PACKED
#endif

/* Attribute pure is used for functions that have no effects other than their
 * return value.  As a result, calls to it can be dead code eliminated.
 */
#ifdef HAVE_FUNC_ATTRIBUTE_PURE
#define ATTRIBUTE_PURE __attribute__((__pure__))
#else
#define ATTRIBUTE_PURE
#endif

#ifdef HAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL
#define ATTRIBUTE_RETURNS_NONNULL __attribute__((__returns_nonnull__))
#else
#define ATTRIBUTE_RETURNS_NONNULL
#endif

#ifdef __cplusplus
/**
 * Macro function that evaluates to true if T is a trivially
 * destructible type -- that is, if its (non-virtual) destructor
 * performs no action and all member variables and base classes are
 * trivially destructible themselves.
 */
#   if (defined(__clang__) && defined(__has_feature))
#      if __has_feature(has_trivial_destructor)
#         define HAS_TRIVIAL_DESTRUCTOR(T) __has_trivial_destructor(T)
#      endif
#   elif defined(__GNUC__)
#      if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
#         define HAS_TRIVIAL_DESTRUCTOR(T) __has_trivial_destructor(T)
#      endif
#   elif defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#      if _MSC_VER >= 1800
#         define HAS_TRIVIAL_DESTRUCTOR(T) __has_trivial_destructor(T)
#      endif
#   endif
#   ifndef HAS_TRIVIAL_DESTRUCTOR
       /* It's always safe (if inefficient) to assume that a
        * destructor is non-trivial.
        */
#      define HAS_TRIVIAL_DESTRUCTOR(T) (false)
#   endif
#endif

/**
 * PUBLIC/USED macros
 *
 * If we build the library with gcc's -fvisibility=hidden flag, we'll
 * use the PUBLIC macro to mark functions that are to be exported.
 *
 * We also need to define a USED attribute, so the optimizer doesn't
 * inline a static function that we later use in an alias. - ajax
 */
#ifndef PUBLIC
#  if defined(__GNUC__)
#    define PUBLIC __attribute__((visibility("default")))
#    define USED __attribute__((used))
#  elif defined(_MSC_VER)
#    define PUBLIC __declspec(dllexport)
#    define USED
#  else
#    define PUBLIC
#    define USED
#  endif
#endif

#ifdef HAVE_FUNC_ATTRIBUTE_UNUSED
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define MAYBE_UNUSED UNUSED

#ifdef HAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT
#define MUST_CHECK __attribute__((warn_unused_result))
#else
#define MUST_CHECK
#endif

#if defined(__GNUC__)
#define ATTRIBUTE_NOINLINE __attribute__((noinline))
#else
#define ATTRIBUTE_NOINLINE
#endif

/** Compute ceiling of integer quotient of A divided by B. */
#define DIV_ROUND_UP( A, B )  ( (A) % (B) == 0 ? (A)/(B) : (A)/(B)+1 )

/** Clamp X to [MIN,MAX].  Turn NaN into MIN, arbitrarily. */
#define CLAMP( X, MIN, MAX )  ( (X)>(MIN) ? ((X)>(MAX) ? (MAX) : (X)) : (MIN) )

/** Minimum of two values: */
#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )

/** Maximum of two values: */
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

/** Minimum and maximum of three values: */
#define MIN3( A, B, C ) ((A) < (B) ? MIN2(A, C) : MIN2(B, C))
#define MAX3( A, B, C ) ((A) > (B) ? MAX2(A, C) : MAX2(B, C))

#endif /* UTIL_MACROS_H */
