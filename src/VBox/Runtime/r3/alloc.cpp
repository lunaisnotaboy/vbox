/* $Id$ */
/** @file
 * IPRT - Memory Allocation.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifdef RTMEM_WRAP_TO_EF_APIS
# undef RTMEM_WRAP_TO_EF_APIS
# define RTALLOC_USE_EFENCE 1
#endif


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "alloc-ef.h"
#include <iprt/alloc.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/param.h>
#include <iprt/string.h>

#include <stdlib.h>

#undef RTMemTmpAlloc
#undef RTMemTmpAllocZ
#undef RTMemTmpFree
#undef RTMemAlloc
#undef RTMemAllocZ
#undef RTMemAllocVar
#undef RTMemAllocZVar
#undef RTMemRealloc
#undef RTMemFree
#undef RTMemDup
#undef RTMemDupEx


/**
 * Allocates temporary memory.
 *
 * Temporary memory blocks are used for not too large memory blocks which
 * are believed not to stick around for too long. Using this API instead
 * of RTMemAlloc() not only gives the heap manager room for optimization
 * but makes the code easier to read.
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure.
 * @param   cb      Size in bytes of the memory block to allocate.
 */
RTDECL(void *)  RTMemTmpAlloc(size_t cb) RT_NO_THROW
{
    return RTMemAlloc(cb);
}


/**
 * Allocates zero'ed temporary memory.
 *
 * Same as RTMemTmpAlloc() but the memory will be zero'ed.
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure.
 * @param   cb      Size in bytes of the memory block to allocate.
 */
RTDECL(void *)  RTMemTmpAllocZ(size_t cb) RT_NO_THROW
{
    return RTMemAllocZ(cb);
}


/**
 * Free temporary memory.
 *
 * @param   pv      Pointer to memory block.
 */
RTDECL(void)    RTMemTmpFree(void *pv) RT_NO_THROW
{
    RTMemFree(pv);
}


/**
 * Allocates memory.
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure.
 * @param   cb      Size in bytes of the memory block to allocate.
 */
RTDECL(void *)  RTMemAlloc(size_t cb) RT_NO_THROW
{
#ifdef RTALLOC_USE_EFENCE
    void *pv = rtR3MemAlloc("Alloc", RTMEMTYPE_RTMEMALLOC, cb, cb, ASMReturnAddress(), NULL, 0, NULL);

#else /* !RTALLOC_USE_EFENCE */

    AssertMsg(cb, ("Allocating ZERO bytes is really not a good idea! Good luck with the next assertion!\n"));
    void *pv = malloc(cb);
    AssertMsg(pv, ("malloc(%#zx) failed!!!\n", cb));
    AssertMsg(   cb < RTMEM_ALIGNMENT
              || !((uintptr_t)pv & (RTMEM_ALIGNMENT - 1))
              || ( (cb & RTMEM_ALIGNMENT) + ((uintptr_t)pv & RTMEM_ALIGNMENT)) == RTMEM_ALIGNMENT
              , ("pv=%p RTMEM_ALIGNMENT=%#x\n", pv, RTMEM_ALIGNMENT));
#endif /* !RTALLOC_USE_EFENCE */
    return pv;
}


/**
 * Allocates zero'ed memory.
 *
 * Instead of memset(pv, 0, sizeof()) use this when you want zero'ed
 * memory. This keeps the code smaller and the heap can skip the memset
 * in about 0.42% of the calls :-).
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure.
 * @param   cb      Size in bytes of the memory block to allocate.
 */
RTDECL(void *)  RTMemAllocZ(size_t cb) RT_NO_THROW
{
#ifdef RTALLOC_USE_EFENCE
    void *pv = rtR3MemAlloc("AllocZ", RTMEMTYPE_RTMEMALLOCZ, cb, cb, ASMReturnAddress(), NULL, 0, NULL);

#else /* !RTALLOC_USE_EFENCE */

    AssertMsg(cb, ("Allocating ZERO bytes is really not a good idea! Good luck with the next assertion!\n"));

    void *pv = calloc(1, cb);
    AssertMsg(pv, ("calloc(1,%#zx) failed!!!\n", cb));
    AssertMsg(   cb < RTMEM_ALIGNMENT
              || !((uintptr_t)pv & (RTMEM_ALIGNMENT - 1))
              || ( (cb & RTMEM_ALIGNMENT) + ((uintptr_t)pv & RTMEM_ALIGNMENT)) == RTMEM_ALIGNMENT
              , ("pv=%p RTMEM_ALIGNMENT=%#x\n", pv, RTMEM_ALIGNMENT));
#endif /* !RTALLOC_USE_EFENCE */
    return pv;
}


/**
 * Wrapper around RTMemAlloc for automatically aligning variable sized
 * allocations so that the various electric fence heaps works correctly.
 *
 * @returns See RTMemAlloc.
 * @param   cbUnaligned         The unaligned size.
 */
RTDECL(void *) RTMemAllocVar(size_t cbUnaligned)
{
    size_t cbAligned;
    if (cbUnaligned >= 16)
        cbAligned = RT_ALIGN_Z(cbUnaligned, 16);
    else
        cbAligned = RT_ALIGN_Z(cbUnaligned, sizeof(void *));
#ifdef RTALLOC_USE_EFENCE
    void *pv = rtR3MemAlloc("AllocVar", RTMEMTYPE_RTMEMALLOC, cbUnaligned, cbAligned, ASMReturnAddress(), NULL, 0, NULL);
#else
    void *pv = RTMemAlloc(cbAligned);
#endif
    return pv;
}


/**
 * Wrapper around RTMemAllocZ for automatically aligning variable sized
 * allocations so that the various electric fence heaps works correctly.
 *
 * @returns See RTMemAllocZ.
 * @param   cbUnaligned         The unaligned size.
 */
RTDECL(void *) RTMemAllocZVar(size_t cbUnaligned)
{
    size_t cbAligned;
    if (cbUnaligned >= 16)
        cbAligned = RT_ALIGN_Z(cbUnaligned, 16);
    else
        cbAligned = RT_ALIGN_Z(cbUnaligned, sizeof(void *));
#ifdef RTALLOC_USE_EFENCE
    void *pv = rtR3MemAlloc("AllocZVar", RTMEMTYPE_RTMEMALLOCZ, cbUnaligned, cbAligned, ASMReturnAddress(), NULL, 0, NULL);
#else
    void *pv = RTMemAllocZ(cbAligned);
#endif
    return pv;
}


/**
 * Reallocates memory.
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure.
 * @param   pvOld   The memory block to reallocate.
 * @param   cbNew   The new block size (in bytes).
 */
RTDECL(void *)  RTMemRealloc(void *pvOld, size_t cbNew) RT_NO_THROW
{
#ifdef RTALLOC_USE_EFENCE
    void *pv = rtR3MemRealloc("Realloc", RTMEMTYPE_RTMEMREALLOC, pvOld, cbNew, ASMReturnAddress(), NULL, 0, NULL);

#else /* !RTALLOC_USE_EFENCE */

    void *pv = realloc(pvOld, cbNew);
    AssertMsg(pv && cbNew, ("realloc(%p, %#zx) failed!!!\n", pvOld, cbNew));
    AssertMsg(   cbNew < RTMEM_ALIGNMENT
              || !((uintptr_t)pv & (RTMEM_ALIGNMENT - 1))
              || ( (cbNew & RTMEM_ALIGNMENT) + ((uintptr_t)pv & RTMEM_ALIGNMENT)) == RTMEM_ALIGNMENT
              , ("pv=%p RTMEM_ALIGNMENT=%#x\n", pv, RTMEM_ALIGNMENT));
#endif  /* !RTALLOC_USE_EFENCE */
    return pv;
}


/**
 * Free memory related to a virtual machine
 *
 * @param   pv      Pointer to memory block.
 */
RTDECL(void)    RTMemFree(void *pv) RT_NO_THROW
{
    if (pv)
#ifdef RTALLOC_USE_EFENCE
        rtR3MemFree("Free", RTMEMTYPE_RTMEMFREE, pv, ASMReturnAddress(), NULL, 0, NULL);
#else
        free(pv);
#endif
}

