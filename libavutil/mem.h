/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * memory handling functions
 */

#ifndef AVUTIL_MEM_H
#define AVUTIL_MEM_H

#include "attributes.h"
#include "error.h"
#include "avutil.h"

#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1110 || defined(__SUNPRO_C)
    #define DECLARE_ALIGNED(n,t,v)      t __attribute__ ((aligned (n))) v
    #define DECLARE_ASM_CONST(n,t,v)    const t __attribute__ ((aligned (n))) v
#elif defined(__TI_COMPILER_VERSION__)
    #define DECLARE_ALIGNED(n,t,v)                      \
        AV_PRAGMA(DATA_ALIGN(v,n))                      \
        t __attribute__((aligned(n))) v
    #define DECLARE_ASM_CONST(n,t,v)                    \
        AV_PRAGMA(DATA_ALIGN(v,n))                      \
        static const t __attribute__((aligned(n))) v
#elif defined(__GNUC__)
    #define DECLARE_ALIGNED(n,t,v)      t __attribute__ ((aligned (n))) v
    #define DECLARE_ASM_CONST(n,t,v)    static const t av_used __attribute__ ((aligned (n))) v
#elif defined(_MSC_VER)
    #define DECLARE_ALIGNED(n,t,v)      __declspec(align(n)) t v
    #define DECLARE_ASM_CONST(n,t,v)    __declspec(align(n)) static const t v
#else
    #define DECLARE_ALIGNED(n,t,v)      t v
    #define DECLARE_ASM_CONST(n,t,v)    static const t v
#endif

#if AV_GCC_VERSION_AT_LEAST(3,1)
    #define av_malloc_attrib __attribute__((__malloc__))
#else
    #define av_malloc_attrib
#endif

#if AV_GCC_VERSION_AT_LEAST(4,3)
    #define av_alloc_size(n) __attribute__((alloc_size(n)))
#else
    #define av_alloc_size(n)
#endif

#if defined(__LGE__) && defined(MALLOC_PREFIX)
#define av_malloc(s) 			av_debug_malloc(s,__FILE__,__FUNCTION__,__LINE__)
#define av_realloc(p,s) 		av_debug_realloc(p,s,__FILE__,__FUNCTION__,__LINE__)
#define av_realloc_f(p,n,e) 	av_debug_realloc_f(p,n,e,__FILE__,__FUNCTION__,__LINE__)
#define av_mallocz(s) 			av_debug_mallocz(s,__FILE__,__FUNCTION__,__LINE__)
#define av_calloc(n,s) 			av_debug_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define av_strdup(s) 			av_debug_strdup(s,__FILE__,__FUNCTION__,__LINE__)
#define av_dynarray_add(t,n,e) 	av_debug_dynarray_add(t,n,e,__FILE__,__FUNCTION__,__LINE__)
#endif

/**
 * Allocate a block of size bytes with alignment suitable for all
 * memory accesses (including vectors if available on the CPU).
 * @param size Size in bytes for the memory block to be allocated.
 * @return Pointer to the allocated block, NULL if the block cannot
 * be allocated.
 * @see av_mallocz()
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_malloc(size_t size, char* sfile, const char* fname, int line) av_malloc_attrib av_alloc_size(4);
#else
void *av_malloc(size_t size) av_malloc_attrib av_alloc_size(1);
#endif

/**
 * Allocate or reallocate a block of memory.
 * If ptr is NULL and size > 0, allocate a new block. If
 * size is zero, free the memory block pointed to by ptr.
 * @param size Size in bytes for the memory block to be allocated or
 * reallocated.
 * @param ptr Pointer to a memory block already allocated with
 * av_malloc(z)() or av_realloc() or NULL.
 * @return Pointer to a newly reallocated block or NULL if the block
 * cannot be reallocated or the function is used to free the memory block.
 * @see av_fast_realloc()
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_realloc(void *ptr, size_t size, char* sfile, const char* fname, int line) av_alloc_size(5);
#else
void *av_realloc(void *ptr, size_t size) av_alloc_size(2);
#endif

/**
 * Allocate or reallocate a block of memory.
 * This function does the same thing as av_realloc, except:
 * - It takes two arguments and checks the result of the multiplication for
 *   integer overflow.
 * - It frees the input block in case of failure, thus avoiding the memory
 *   leak with the classic "buf = realloc(buf); if (!buf) return -1;".
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_realloc_f(void *ptr, size_t nelem, size_t elsize, char* sfile, const char* fname, int line);
#else
void *av_realloc_f(void *ptr, size_t nelem, size_t elsize);
#endif

/**
 * Free a memory block which has been allocated with av_malloc(z)() or
 * av_realloc().
 * @param ptr Pointer to the memory block which should be freed.
 * @note ptr = NULL is explicitly allowed.
 * @note It is recommended that you use av_freep() instead.
 * @see av_freep()
 */
void av_free(void *ptr);

/**
 * Allocate a block of size bytes with alignment suitable for all
 * memory accesses (including vectors if available on the CPU) and
 * zero all the bytes of the block.
 * @param size Size in bytes for the memory block to be allocated.
 * @return Pointer to the allocated block, NULL if it cannot be allocated.
 * @see av_malloc()
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_mallocz(size_t size, char* sfile, const char* fname, int line) av_malloc_attrib av_alloc_size(4);
#else
void *av_mallocz(size_t size) av_malloc_attrib av_alloc_size(1);
#endif

/**
 * Allocate a block of nmemb * size bytes with alignment suitable for all
 * memory accesses (including vectors if available on the CPU) and
 * zero all the bytes of the block.
 * The allocation will fail if nmemb * size is greater than or equal
 * to INT_MAX.
 * @param nmemb
 * @param size
 * @return Pointer to the allocated block, NULL if it cannot be allocated.
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_calloc(size_t nmemb, size_t size, char* sfile, const char* fname, int line) av_malloc_attrib;
#else
void *av_calloc(size_t nmemb, size_t size) av_malloc_attrib;
#endif

/**
 * Duplicate the string s.
 * @param s string to be duplicated
 * @return Pointer to a newly allocated string containing a
 * copy of s or NULL if the string cannot be allocated.
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
char *av_debug_strdup(const char *s, char* sfile, const char* fname, int line) av_malloc_attrib;
#else
char *av_strdup(const char *s) av_malloc_attrib;
#endif

/**
 * Free a memory block which has been allocated with av_malloc(z)() or
 * av_realloc() and set the pointer pointing to it to NULL.
 * @param ptr Pointer to the pointer to the memory block which should
 * be freed.
 * @see av_free()
 */
void av_freep(void *ptr);

/**
 * Add an element to a dynamic array.
 *
 * @param tab_ptr Pointer to the array.
 * @param nb_ptr  Pointer to the number of elements in the array.
 * @param elem    Element to be added.
 */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void av_debug_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem, char* sfile, const char* fname, int line);
#else
void av_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem);
#endif

/**
 * Multiply two size_t values checking for overflow.
 * @return  0 if success, AVERROR(EINVAL) if overflow.
 */
static inline int av_size_mult(size_t a, size_t b, size_t *r)
{
    size_t t = a * b;
    /* Hack inspired from glibc: only try the division if nelem and elsize
     * are both greater than sqrt(SIZE_MAX). */
    if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
        return AVERROR(EINVAL);
    *r = t;
    return 0;
}

#endif /* AVUTIL_MEM_H */
