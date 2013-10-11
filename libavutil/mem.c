/*
 * default memory allocator for libavutil
 * Copyright (c) 2002 Fabrice Bellard
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
 * default memory allocator for libavutil
 */

#define _XOPEN_SOURCE 600

#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "avutil.h"
#include "mem.h"

/* here we can use OS-dependent allocation functions */
#undef free
#undef malloc
#undef realloc

#ifdef __LGE__
// defined in ffmpeg.c
#define FFMPEG_EXIT_CODE_NONE       (-1)
#define FFMPEG_EXIT_CODE_NORMAL     (0)
#define FFMPEG_EXIT_CODE_ERROR      (1)
#define FFMPEG_EXIT_CODE_CANCEL     (2)
#define FFMPEG_EXIT_CODE_NOMEM      (3)
#endif

#ifdef __LGE__
#define AV_SET_NOMEM() do{ \
			extern int ffmpeg_return(int ret); \
            av_log(NULL, AV_LOG_ERROR, "AV_SET_NOMEM --> called at %s:%d \n", __FUNCTION__, __LINE__); \
			ffmpeg_return(FFMPEG_EXIT_CODE_NOMEM); \
		}while(0)
#endif

#ifdef MALLOC_PREFIX

#define malloc         AV_JOIN(MALLOC_PREFIX, malloc)
#define memalign       AV_JOIN(MALLOC_PREFIX, memalign)
#define posix_memalign AV_JOIN(MALLOC_PREFIX, posix_memalign)
#define realloc        AV_JOIN(MALLOC_PREFIX, realloc)
#define free           AV_JOIN(MALLOC_PREFIX, free)

#ifdef __LGE__
void *malloc(size_t size, char* sfile, const char* fname, int line);
void *memalign(size_t align, size_t size, char* sfile, const char* fname, int line);
int   posix_memalign(void **ptr, size_t align, size_t size, char* sfile, const char* fname, int line);
void *realloc(void *ptr, size_t size, char* sfile, const char* fname, int line);
void  free(void *ptr);
#else
void *malloc(size_t size);
void *memalign(size_t align, size_t size);
int   posix_memalign(void **ptr, size_t align, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);
#endif

#endif /* MALLOC_PREFIX */

#define ALIGN (HAVE_AVX ? 32 : 16)

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

#define MAX_MALLOC_SIZE INT_MAX

#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_malloc(size_t size, char* sfile, const char* fname, int line)
#else
void *av_malloc(size_t size)
#endif
{
    void *ptr = NULL;
#if CONFIG_MEMALIGN_HACK
    long diff;
#endif

    /* let's disallow possible ambiguous cases */
    if (size > (MAX_MALLOC_SIZE-32))
#ifdef __LGE__
	{
		AV_SET_NOMEM();
        return NULL;
	}
#else
        return NULL;
#endif

#if CONFIG_MEMALIGN_HACK
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    ptr = malloc(size+ALIGN, sfile, fname, line);
#else
    ptr = malloc(size+ALIGN);
#endif
    if(!ptr)
#ifdef __LGE__
	{
		AV_SET_NOMEM();
        return ptr;
	}
#else
        return ptr;
#endif
    diff= ((-(long)ptr - 1)&(ALIGN-1)) + 1;
    ptr = (char*)ptr + diff;
    ((char*)ptr)[-1]= diff;
#elif HAVE_POSIX_MEMALIGN
    if (size) //OSX on SDK 10.6 has a broken posix_memalign implementation
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    if (posix_memalign(&ptr,ALIGN,size, sfile, fname, line))
#else
    if (posix_memalign(&ptr,ALIGN,size))
#endif
        ptr = NULL;
#elif HAVE_MEMALIGN
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    ptr = memalign(ALIGN,size, sfile, fname, line);
#else
    ptr = memalign(ALIGN,size);
#endif
    /* Why 64?
       Indeed, we should align it:
         on 4 for 386
         on 16 for 486
         on 32 for 586, PPro - K6-III
         on 64 for K7 (maybe for P3 too).
       Because L1 and L2 caches are aligned on those values.
       But I don't want to code such logic here!
     */
     /* Why 32?
        For AVX ASM. SSE / NEON needs only 16.
        Why not larger? Because I did not see a difference in benchmarks ...
     */
     /* benchmarks with P3
        memalign(64)+1          3071,3051,3032
        memalign(64)+2          3051,3032,3041
        memalign(64)+4          2911,2896,2915
        memalign(64)+8          2545,2554,2550
        memalign(64)+16         2543,2572,2563
        memalign(64)+32         2546,2545,2571
        memalign(64)+64         2570,2533,2558

        BTW, malloc seems to do 8-byte alignment by default here.
     */
#else
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    ptr = malloc(size, sfile, fname, line);
#else
    ptr = malloc(size);
#endif
#endif
    if(!ptr && !size)
#if defined(__LGE__) && defined(MALLOC_PREFIX)
        ptr= av_debug_malloc(1, sfile, fname, line);
#else
        ptr= av_malloc(1);
#endif
#ifdef __LGE__
	if (ptr == NULL)
		AV_SET_NOMEM();
#endif
    return ptr;
}

#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_realloc(void *ptr, size_t size, char* sfile, const char* fname, int line)
#else
void *av_realloc(void *ptr, size_t size)
#endif
{
#if CONFIG_MEMALIGN_HACK
    int diff;
#endif

    /* let's disallow possible ambiguous cases */
    if (size > (MAX_MALLOC_SIZE-16))
#ifdef __LGE__
	{
		AV_SET_NOMEM();
        return NULL;
	}
#else
        return NULL;
#endif

#if CONFIG_MEMALIGN_HACK
    //FIXME this isn't aligned correctly, though it probably isn't needed
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    if(!ptr) return av_debug_malloc(size, sfile, fname, line);
#else
    if(!ptr) return av_malloc(size);
#endif
    diff= ((char*)ptr)[-1];
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    ptr= realloc((char*)ptr - diff, size + diff, sfile, fname, line);
#else
    ptr= realloc((char*)ptr - diff, size + diff);
#endif
    if(ptr) ptr = (char*)ptr + diff;
#ifdef __LGE__
	if (ptr == NULL)
		AV_SET_NOMEM();
#endif
    return ptr;
#else
#if defined(__LGE__) && defined(MALLOC_PREFIX)
	{
		void* ret =	realloc(ptr, size + !size, sfile, fname, line);
		if (ret == NULL)
			AV_SET_NOMEM();
		return ret;
	}
#else
#ifdef __LGE__
	{
		void* ret =	realloc(ptr, size + !size);
		if (ret == NULL)
			AV_SET_NOMEM();
		return ret;
	}
#else
    return realloc(ptr, size + !size);
#endif
#endif
#endif
}

#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_realloc_f(void *ptr, size_t nelem, size_t elsize, char* sfile, const char* fname, int line)
#else
void *av_realloc_f(void *ptr, size_t nelem, size_t elsize)
#endif
{
    size_t size;
    void *r;

    if (av_size_mult(elsize, nelem, &size)) {
        av_free(ptr);
#ifdef __LGE__
		AV_SET_NOMEM();
#endif
        return NULL;
    }
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    r = av_debug_realloc(ptr, size, sfile, fname, line);
#else
    r = av_realloc(ptr, size);
#endif
    if (!r && size)
        av_free(ptr);
#ifdef __LGE__
	if (r == NULL)
		AV_SET_NOMEM();
#endif
    return r;
}

void av_free(void *ptr)
{
#if CONFIG_MEMALIGN_HACK
    if (ptr)
        free((char*)ptr - ((char*)ptr)[-1]);
#else
    free(ptr);
#endif
}

void av_freep(void *arg)
{
    void **ptr= (void**)arg;
    av_free(*ptr);
    *ptr = NULL;
}

#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_mallocz(size_t size, char* sfile, const char* fname, int line)
#else
void *av_mallocz(size_t size)
#endif
{
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    void *ptr = av_debug_malloc(size, sfile, fname, line);
#else
    void *ptr = av_malloc(size);
#endif
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

#if defined(__LGE__) && defined(MALLOC_PREFIX)
void *av_debug_calloc(size_t nmemb, size_t size, char* sfile, const char* fname, int line)
#else
void *av_calloc(size_t nmemb, size_t size)
#endif
{
    if (size <= 0 || nmemb >= INT_MAX / size)
#ifdef __LGE__
	{
		AV_SET_NOMEM();
        return NULL;
	}
#else
        return NULL;
#endif
#if defined(__LGE__) && defined(MALLOC_PREFIX)
    return av_debug_mallocz(nmemb * size, sfile, fname, line);
#else
    return av_mallocz(nmemb * size);
#endif
}

#if defined(__LGE__) && defined(MALLOC_PREFIX)
char *av_debug_strdup(const char *s, char* sfile, const char* fname, int line)
#else
char *av_strdup(const char *s)
#endif
{
    char *ptr= NULL;
    if(s){
        int len = strlen(s) + 1;
#if defined(__LGE__) && defined(MALLOC_PREFIX)
        ptr = av_debug_malloc(len, sfile, fname, line);
#else
        ptr = av_malloc(len);
#endif
#ifdef __LGE__
		if (ptr == NULL)
			AV_SET_NOMEM();
#endif
        if (ptr)
            memcpy(ptr, s, len);
    }
    return ptr;
}

/* add one element to a dynamic array */
#if defined(__LGE__) && defined(MALLOC_PREFIX)
void av_debug_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem, char* sfile, const char* fname, int line)
#else
void av_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem)
#endif
{
    /* see similar ffmpeg.c:grow_array() */
    int nb, nb_alloc;
    intptr_t *tab;

    nb = *nb_ptr;
    tab = *(intptr_t**)tab_ptr;
    if ((nb & (nb - 1)) == 0) {
        if (nb == 0)
            nb_alloc = 1;
        else
            nb_alloc = nb * 2;
#if defined(__LGE__) && defined(MALLOC_PREFIX)
        tab = av_debug_realloc(tab, nb_alloc * sizeof(intptr_t), sfile, fname, line);
#else
        tab = av_realloc(tab, nb_alloc * sizeof(intptr_t));
#endif
#ifdef __LGE__
		if (tab == NULL)
			return;
#endif
        *(intptr_t**)tab_ptr = tab;
    }
    tab[nb++] = (intptr_t)elem;
    *nb_ptr = nb;
}
