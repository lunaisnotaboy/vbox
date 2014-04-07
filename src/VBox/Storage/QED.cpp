/* $Id$ */
/** @file
 * QED - QED Disk image.
 */

/*
 * Copyright (C) 2011-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_VD_QED
#include <VBox/vd-plugin.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/alloc.h>
#include <iprt/path.h>
#include <iprt/list.h>

#include "VDBackends.h"

/**
 * The QED backend implements support for the qemu enhanced disk format (short QED)
 * The specification for the format is available under http://wiki.qemu.org/Features/QED/Specification
 *
 * Missing things to implement:
 *    - compaction
 *    - resizing which requires block relocation (very rare case)
 */

/*******************************************************************************
*   Structures in a QED image, little endian                                   *
*******************************************************************************/

#pragma pack(1)
typedef struct QedHeader
{
    /** Magic value. */
    uint32_t    u32Magic;
    /** Cluster size in bytes. */
    uint32_t    u32ClusterSize;
    /** Size of L1 and L2 tables in clusters. */
    uint32_t    u32TableSize;
    /** size of this header structure in clusters. */
    uint32_t    u32HeaderSize;
    /** Features used for the image. */
    uint64_t    u64FeatureFlags;
    /** Compatibility features used for the image. */
    uint64_t    u64CompatFeatureFlags;
    /** Self resetting feature bits. */
    uint64_t    u64AutoresetFeatureFlags;
    /** Offset of the L1 table in bytes. */
    uint64_t    u64OffL1Table;
    /** Logical image size as seen by the guest. */
    uint64_t    u64Size;
    /** Offset of the backing filename in bytes. */
    uint32_t    u32OffBackingFilename;
    /** Size of the backing filename. */
    uint32_t    u32BackingFilenameSize;
} QedHeader;
#pragma pack()
/** Pointer to a on disk QED header. */
typedef QedHeader *PQedHeader;

/** QED magic value. */
#define QED_MAGIC                            UINT32_C(0x00444551) /* QED\0 */
/** Cluster size minimum. */
#define QED_CLUSTER_SIZE_MIN                 RT_BIT(12)
/** Cluster size maximum. */
#define QED_CLUSTER_SIZE_MAX                 RT_BIT(26)
/** L1 and L2 Table size minimum. */
#define QED_TABLE_SIZE_MIN                   1
/** L1 and L2 Table size maximum. */
#define QED_TABLE_SIZE_MAX                   16

/** QED default cluster size when creating an image. */
#define QED_CLUSTER_SIZE_DEFAULT             (64 * _1K)
/** The default table size in clusters. */
#define QED_TABLE_SIZE_DEFAULT               4

/** Feature flags.
 * @{
 */
/** Image uses a backing file to provide data for unallocated clusters. */
#define QED_FEATURE_BACKING_FILE             RT_BIT_64(0)
/** Image needs checking before use. */
#define QED_FEATURE_NEED_CHECK               RT_BIT_64(1)
/** Don't probe for format of the backing file, treat as raw image. */
#define QED_FEATURE_BACKING_FILE_NO_PROBE    RT_BIT_64(2)
/** Mask of valid features. */
#define QED_FEATURE_MASK (QED_FEATURE_BACKING_FILE | QED_FEATURE_NEED_CHECK | QED_FEATURE_BACKING_FILE_NO_PROBE)
/** @} */

/** Compatibility feature flags.
 * @{
 */
/** Mask of valid compatibility features. */
#define QED_COMPAT_FEATURE_MASK (0)
/** @} */

/** Autoreset feature flags.
 * @{
 */
/** Mask of valid autoreset features. */
#define QED_AUTORESET_FEATURE_MASK (0)
/** @} */

/*******************************************************************************
*   Constants And Macros, Structures and Typedefs                              *
*******************************************************************************/

/**
 * QED L2 cache entry.
 */
typedef struct QEDL2CACHEENTRY
{
    /** List node for the search list. */
    RTLISTNODE              NodeSearch;
    /** List node for the LRU list. */
    RTLISTNODE              NodeLru;
    /** Reference counter. */
    uint32_t                cRefs;
    /** The offset of the L2 table, used as search key. */
    uint64_t                offL2Tbl;
    /** Pointer to the cached L2 table. */
    uint64_t               *paL2Tbl;
} QEDL2CACHEENTRY, *PQEDL2CACHEENTRY;

/** Maximum amount of memory the cache is allowed to use. */
#define QED_L2_CACHE_MEMORY_MAX (2*_1M)

/**
 * QED image data structure.
 */
typedef struct QEDIMAGE
{
    /** Image name. */
    const char          *pszFilename;
    /** Storage handle. */
    PVDIOSTORAGE        pStorage;

    /** Pointer to the per-disk VD interface list. */
    PVDINTERFACE        pVDIfsDisk;
    /** Pointer to the per-image VD interface list. */
    PVDINTERFACE        pVDIfsImage;
    /** Error interface. */
    PVDINTERFACEERROR   pIfError;
    /** I/O interface. */
    PVDINTERFACEIOINT   pIfIo;

    /** Open flags passed by VBoxHD layer. */
    unsigned            uOpenFlags;
    /** Image flags defined during creation or determined during open. */
    unsigned            uImageFlags;
    /** Total size of the image. */
    uint64_t            cbSize;
    /** Physical geometry of this image. */
    VDGEOMETRY          PCHSGeometry;
    /** Logical geometry of this image. */
    VDGEOMETRY          LCHSGeometry;

    /** Filename of the backing file if any. */
    char               *pszBackingFilename;
    /** Offset of the filename in the image. */
    uint32_t            offBackingFilename;
    /** Size of the backing filename excluding \0. */
    uint32_t            cbBackingFilename;

    /** Size of the image, multiple of clusters. */
    uint64_t            cbImage;
    /** Cluster size in bytes. */
    uint32_t            cbCluster;
    /** Number of entries in the L1 and L2 table. */
    uint32_t            cTableEntries;
    /** Size of an L1 or L2 table rounded to the next cluster size. */
    uint32_t            cbTable;
    /** Pointer to the L1 table. */
    uint64_t            *paL1Table;
    /** Offset of the L1 table. */
    uint64_t            offL1Table;

    /** Offset mask for a cluster. */
    uint64_t            fOffsetMask;
    /** L1 table mask to get the L1 index. */
    uint64_t            fL1Mask;
    /** Number of bits to shift to get the L1 index. */
    uint32_t            cL1Shift;
    /** L2 table mask to get the L2 index. */
    uint64_t            fL2Mask;
    /** Number of bits to shift to get the L2 index. */
    uint32_t            cL2Shift;

    /** Memory occupied by the L2 table cache. */
    size_t              cbL2Cache;
    /** The sorted L2 entry list used for searching. */
    RTLISTNODE          ListSearch;
    /** The LRU L2 entry list used for eviction. */
    RTLISTNODE          ListLru;

} QEDIMAGE, *PQEDIMAGE;

/**
 * State of the async cluster allocation.
 */
typedef enum QEDCLUSTERASYNCALLOCSTATE
{
    /** Invalid. */
    QEDCLUSTERASYNCALLOCSTATE_INVALID = 0,
    /** L2 table allocation. */
    QEDCLUSTERASYNCALLOCSTATE_L2_ALLOC,
    /** Link L2 table into L1. */
    QEDCLUSTERASYNCALLOCSTATE_L2_LINK,
    /** Allocate user data cluster. */
    QEDCLUSTERASYNCALLOCSTATE_USER_ALLOC,
    /** Link user data cluster. */
    QEDCLUSTERASYNCALLOCSTATE_USER_LINK,
    /** 32bit blowup. */
    QEDCLUSTERASYNCALLOCSTATE_32BIT_HACK = 0x7fffffff
} QEDCLUSTERASYNCALLOCSTATE, *PQEDCLUSTERASYNCALLOCSTATE;

/**
 * Data needed to track async cluster allocation.
 */
typedef struct QEDCLUSTERASYNCALLOC
{
    /** The state of the cluster allocation. */
    QEDCLUSTERASYNCALLOCSTATE enmAllocState;
    /** Old image size to rollback in case of an error. */
    uint64_t                  cbImageOld;
    /** L1 index to link if any. */
    uint32_t                  idxL1;
    /** L2 index to link, required in any case. */
    uint32_t                  idxL2;
    /** Start offset of the allocated cluster. */
    uint64_t                  offClusterNew;
    /** L2 cache entry if a L2 table is allocated. */
    PQEDL2CACHEENTRY          pL2Entry;
    /** Number of bytes to write. */
    size_t                    cbToWrite;
} QEDCLUSTERASYNCALLOC, *PQEDCLUSTERASYNCALLOC;

/*******************************************************************************
*   Static Variables                                                           *
*******************************************************************************/

/** NULL-terminated array of supported file extensions. */
static const VDFILEEXTENSION s_aQedFileExtensions[] =
{
    {"qed", VDTYPE_HDD},
    {NULL,  VDTYPE_INVALID}
};

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/

/**
 * Converts the image header to the host endianess and performs basic checks.
 *
 * @returns Whether the given header is valid or not.
 * @param   pHeader    Pointer to the header to convert.
 */
static bool qedHdrConvertToHostEndianess(PQedHeader pHeader)
{
    pHeader->u32Magic                 = RT_LE2H_U32(pHeader->u32Magic);
    pHeader->u32ClusterSize           = RT_LE2H_U32(pHeader->u32ClusterSize);
    pHeader->u32TableSize             = RT_LE2H_U32(pHeader->u32TableSize);
    pHeader->u32HeaderSize            = RT_LE2H_U32(pHeader->u32HeaderSize);
    pHeader->u64FeatureFlags          = RT_LE2H_U64(pHeader->u64FeatureFlags);
    pHeader->u64CompatFeatureFlags    = RT_LE2H_U64(pHeader->u64CompatFeatureFlags);
    pHeader->u64AutoresetFeatureFlags = RT_LE2H_U64(pHeader->u64AutoresetFeatureFlags);
    pHeader->u64OffL1Table            = RT_LE2H_U64(pHeader->u64OffL1Table);
    pHeader->u64Size                  = RT_LE2H_U64(pHeader->u64Size);
    pHeader->u32OffBackingFilename    = RT_LE2H_U32(pHeader->u32OffBackingFilename);
    pHeader->u32BackingFilenameSize   = RT_LE2H_U32(pHeader->u32BackingFilenameSize);

    if (RT_UNLIKELY(pHeader->u32Magic != QED_MAGIC))
        return false;
    if (RT_UNLIKELY(   pHeader->u32ClusterSize < QED_CLUSTER_SIZE_MIN
                    || pHeader->u32ClusterSize > QED_CLUSTER_SIZE_MAX))
        return false;
    if (RT_UNLIKELY(   pHeader->u32TableSize < QED_TABLE_SIZE_MIN
                    || pHeader->u32TableSize > QED_TABLE_SIZE_MAX))
        return false;
    if (RT_UNLIKELY(pHeader->u64Size % 512 != 0))
        return false;

    return true;
}

/**
 * Creates a QED header from the given image state.
 *
 * @returns nothing.
 * @param   pImage     Image instance data.
 * @param   pHeader    Pointer to the header to convert.
 */
static void qedHdrConvertFromHostEndianess(PQEDIMAGE pImage, PQedHeader pHeader)
{
    pHeader->u32Magic                 = RT_H2LE_U32(QED_MAGIC);
    pHeader->u32ClusterSize           = RT_H2LE_U32(pImage->cbCluster);
    pHeader->u32TableSize             = RT_H2LE_U32(pImage->cbTable / pImage->cbCluster);
    pHeader->u32HeaderSize            = RT_H2LE_U32(1);
    pHeader->u64FeatureFlags          = RT_H2LE_U64(pImage->pszBackingFilename ? QED_FEATURE_BACKING_FILE : UINT64_C(0));
    pHeader->u64CompatFeatureFlags    = RT_H2LE_U64(UINT64_C(0));
    pHeader->u64AutoresetFeatureFlags = RT_H2LE_U64(UINT64_C(0));
    pHeader->u64OffL1Table            = RT_H2LE_U64(pImage->offL1Table);
    pHeader->u64Size                  = RT_H2LE_U64(pImage->cbSize);
    pHeader->u32OffBackingFilename    = RT_H2LE_U32(pImage->offBackingFilename);
    pHeader->u32BackingFilenameSize   = RT_H2LE_U32(pImage->cbBackingFilename);
}

/**
 * Convert table entries from little endian to host endianess.
 *
 * @returns nothing.
 * @param   paTbl       Pointer to the table.
 * @param   cEntries    Number of entries in the table.
 */
static void qedTableConvertToHostEndianess(uint64_t *paTbl, uint32_t cEntries)
{
    while(cEntries-- > 0)
    {
        *paTbl = RT_LE2H_U64(*paTbl);
        paTbl++;
    }
}

/**
 * Convert table entries from host to little endian format.
 *
 * @returns nothing.
 * @param   paTblImg    Pointer to the table which will store the little endian table.
 * @param   paTbl       The source table to convert.
 * @param   cEntries    Number of entries in the table.
 */
static void qedTableConvertFromHostEndianess(uint64_t *paTblImg, uint64_t *paTbl,
                                             uint32_t cEntries)
{
    while(cEntries-- > 0)
    {
        *paTblImg = RT_H2LE_U64(*paTbl);
        paTbl++;
        paTblImg++;
    }
}

/**
 * Creates the L2 table cache.
 *
 * @returns VBox status code.
 * @param   pImage    The image instance data.
 */
static int qedL2TblCacheCreate(PQEDIMAGE pImage)
{
    pImage->cbL2Cache = 0;
    RTListInit(&pImage->ListSearch);
    RTListInit(&pImage->ListLru);

    return VINF_SUCCESS;
}

/**
 * Destroys the L2 table cache.
 *
 * @returns nothing.
 * @param   pImage    The image instance data.
 */
static void qedL2TblCacheDestroy(PQEDIMAGE pImage)
{
    PQEDL2CACHEENTRY pL2Entry = NULL;
    PQEDL2CACHEENTRY pL2Next  = NULL;

    RTListForEachSafe(&pImage->ListSearch, pL2Entry, pL2Next, QEDL2CACHEENTRY, NodeSearch)
    {
        Assert(!pL2Entry->cRefs);

        RTListNodeRemove(&pL2Entry->NodeSearch);
        RTMemPageFree(pL2Entry->paL2Tbl, pImage->cbTable);
        RTMemFree(pL2Entry);
    }

    pImage->cbL2Cache       = 0;
    RTListInit(&pImage->ListSearch);
    RTListInit(&pImage->ListLru);
}

/**
 * Returns the L2 table matching the given offset or NULL if none could be found.
 *
 * @returns Pointer to the L2 table cache entry or NULL.
 * @param   pImage    The image instance data.
 * @param   offL2Tbl  Offset of the L2 table to search for.
 */
static PQEDL2CACHEENTRY qedL2TblCacheRetain(PQEDIMAGE pImage, uint64_t offL2Tbl)
{
    PQEDL2CACHEENTRY pL2Entry = NULL;

    RTListForEach(&pImage->ListSearch, pL2Entry, QEDL2CACHEENTRY, NodeSearch)
    {
        if (pL2Entry->offL2Tbl == offL2Tbl)
            break;
    }

    if (!RTListNodeIsDummy(&pImage->ListSearch, pL2Entry, QEDL2CACHEENTRY, NodeSearch))
    {
        /* Update LRU list. */
        RTListNodeRemove(&pL2Entry->NodeLru);
        RTListPrepend(&pImage->ListLru, &pL2Entry->NodeLru);
        pL2Entry->cRefs++;
        return pL2Entry;
    }
    else
        return NULL;
}

/**
 * Releases a L2 table cache entry.
 *
 * @returns nothing.
 * @param   pL2Entry    The L2 cache entry.
 */
static void qedL2TblCacheEntryRelease(PQEDL2CACHEENTRY pL2Entry)
{
    Assert(pL2Entry->cRefs > 0);
    pL2Entry->cRefs--;
}

/**
 * Allocates a new L2 table from the cache evicting old entries if required.
 *
 * @returns Pointer to the L2 cache entry or NULL.
 * @param   pImage    The image instance data.
 */
static PQEDL2CACHEENTRY qedL2TblCacheEntryAlloc(PQEDIMAGE pImage)
{
    PQEDL2CACHEENTRY pL2Entry = NULL;
    int rc = VINF_SUCCESS;

    if (pImage->cbL2Cache + pImage->cbTable <= QED_L2_CACHE_MEMORY_MAX)
    {
        /* Add a new entry. */
        pL2Entry = (PQEDL2CACHEENTRY)RTMemAllocZ(sizeof(QEDL2CACHEENTRY));
        if (pL2Entry)
        {
            pL2Entry->paL2Tbl = (uint64_t *)RTMemPageAllocZ(pImage->cbTable);
            if (RT_UNLIKELY(!pL2Entry->paL2Tbl))
            {
                RTMemFree(pL2Entry);
                pL2Entry = NULL;
            }
            else
            {
                pL2Entry->cRefs    = 1;
                pImage->cbL2Cache += pImage->cbTable;
            }
        }
    }
    else
    {
        /* Evict the last not in use entry and use it */
        Assert(!RTListIsEmpty(&pImage->ListLru));

        RTListForEachReverse(&pImage->ListLru, pL2Entry, QEDL2CACHEENTRY, NodeLru)
        {
            if (!pL2Entry->cRefs)
                break;
        }

        if (!RTListNodeIsDummy(&pImage->ListSearch, pL2Entry, QEDL2CACHEENTRY, NodeSearch))
        {
            RTListNodeRemove(&pL2Entry->NodeSearch);
            RTListNodeRemove(&pL2Entry->NodeLru);
            pL2Entry->offL2Tbl = 0;
            pL2Entry->cRefs    = 1;
        }
        else
            pL2Entry = NULL;
    }

    return pL2Entry;
}

/**
 * Frees a L2 table cache entry.
 *
 * @returns nothing.
 * @param   pImage    The image instance data.
 * @param   pL2Entry  The L2 cache entry to free.
 */
static void qedL2TblCacheEntryFree(PQEDIMAGE pImage, PQEDL2CACHEENTRY pL2Entry)
{
    Assert(!pL2Entry->cRefs);
    RTMemPageFree(pL2Entry->paL2Tbl, pImage->cbTable);
    RTMemFree(pL2Entry);

    pImage->cbL2Cache -= pImage->cbTable;
}

/**
 * Inserts an entry in the L2 table cache.
 *
 * @returns nothing.
 * @param   pImage    The image instance data.
 * @param   pL2Entry  The L2 cache entry to insert.
 */
static void qedL2TblCacheEntryInsert(PQEDIMAGE pImage, PQEDL2CACHEENTRY pL2Entry)
{
    PQEDL2CACHEENTRY pIt = NULL;

    Assert(pL2Entry->offL2Tbl > 0);

    /* Insert at the top of the LRU list. */
    RTListPrepend(&pImage->ListLru, &pL2Entry->NodeLru);

    if (RTListIsEmpty(&pImage->ListSearch))
    {
        RTListAppend(&pImage->ListSearch, &pL2Entry->NodeSearch);
    }
    else
    {
        /* Insert into search list. */
        pIt = RTListGetFirst(&pImage->ListSearch, QEDL2CACHEENTRY, NodeSearch);
        if (pIt->offL2Tbl > pL2Entry->offL2Tbl)
            RTListPrepend(&pImage->ListSearch, &pL2Entry->NodeSearch);
        else
        {
            bool fInserted = false;

            RTListForEach(&pImage->ListSearch, pIt, QEDL2CACHEENTRY, NodeSearch)
            {
                Assert(pIt->offL2Tbl != pL2Entry->offL2Tbl);
                if (pIt->offL2Tbl < pL2Entry->offL2Tbl)
                {
                    RTListNodeInsertAfter(&pIt->NodeSearch, &pL2Entry->NodeSearch);
                    fInserted = true;
                    break;
                }
            }
             Assert(fInserted);
        }
    }
}

/**
 * Fetches the L2 from the given offset trying the LRU cache first and
 * reading it from the image after a cache miss.
 *
 * @returns VBox status code.
 * @param   pImage    Image instance data.
 * @param   offL2Tbl  The offset of the L2 table in the image.
 * @param   ppL2Entry Where to store the L2 table on success.
 */
static int qedL2TblCacheFetch(PQEDIMAGE pImage, uint64_t offL2Tbl, PQEDL2CACHEENTRY *ppL2Entry)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("pImage=%#p offL2Tbl=%llu ppL2Entry=%#p\n", pImage, offL2Tbl, ppL2Entry));

    /* Try to fetch the L2 table from the cache first. */
    PQEDL2CACHEENTRY pL2Entry = qedL2TblCacheRetain(pImage, offL2Tbl);
    if (!pL2Entry)
    {
        LogFlowFunc(("Reading L2 table from image\n"));
        pL2Entry = qedL2TblCacheEntryAlloc(pImage);

        if (pL2Entry)
        {
            /* Read from the image. */
            pL2Entry->offL2Tbl = offL2Tbl;
            rc = vdIfIoIntFileReadSync(pImage->pIfIo, pImage->pStorage, offL2Tbl,
                                       pL2Entry->paL2Tbl, pImage->cbTable);
            if (RT_SUCCESS(rc))
            {
#if defined(RT_BIG_ENDIAN)
                qedTableConvertToHostEndianess(pL2Entry->paL2Tbl, pImage->cTableEntries);
#endif
                qedL2TblCacheEntryInsert(pImage, pL2Entry);
            }
            else
            {
                qedL2TblCacheEntryRelease(pL2Entry);
                qedL2TblCacheEntryFree(pImage, pL2Entry);
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
        *ppL2Entry = pL2Entry;

    LogFlowFunc(("returns rc=%Rrc\n", rc));
    return rc;
}

/**
 * Fetches the L2 from the given offset trying the LRU cache first and
 * reading it from the image after a cache miss - version for async I/O.
 *
 * @returns VBox status code.
 * @param   pImage    Image instance data.
 * @param   pIoCtx    The I/O context.
 * @param   offL2Tbl  The offset of the L2 table in the image.
 * @param   ppL2Entry Where to store the L2 table on success.
 */
static int qedL2TblCacheFetchAsync(PQEDIMAGE pImage, PVDIOCTX pIoCtx,
                                   uint64_t offL2Tbl, PQEDL2CACHEENTRY *ppL2Entry)
{
    int rc = VINF_SUCCESS;

    /* Try to fetch the L2 table from the cache first. */
    PQEDL2CACHEENTRY pL2Entry = qedL2TblCacheRetain(pImage, offL2Tbl);
    if (!pL2Entry)
    {
        pL2Entry = qedL2TblCacheEntryAlloc(pImage);

        if (pL2Entry)
        {
            /* Read from the image. */
            PVDMETAXFER pMetaXfer;

            pL2Entry->offL2Tbl = offL2Tbl;
            rc = vdIfIoIntFileReadMeta(pImage->pIfIo, pImage->pStorage,
                                       offL2Tbl, pL2Entry->paL2Tbl,
                                       pImage->cbTable, pIoCtx,
                                       &pMetaXfer, NULL, NULL);
            if (RT_SUCCESS(rc))
            {
                vdIfIoIntMetaXferRelease(pImage->pIfIo, pMetaXfer);
#if defined(RT_BIG_ENDIAN)
                qedTableConvertToHostEndianess(pL2Entry->paL2Tbl, pImage->cTableEntries);
#endif
                qedL2TblCacheEntryInsert(pImage, pL2Entry);
            }
            else
            {
                qedL2TblCacheEntryRelease(pL2Entry);
                qedL2TblCacheEntryFree(pImage, pL2Entry);
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
        *ppL2Entry = pL2Entry;

    return rc;
}

/**
 * Return power of 2 or 0 if num error.
 *
 * @returns The power of 2 or 0 if the given number is not a power of 2.
 * @param   u32    The number.
 */
static uint32_t qedGetPowerOfTwo(uint32_t u32)
{
    if (u32 == 0)
        return 0;
    uint32_t uPower2 = 0;
    while ((u32 & 1) == 0)
    {
        u32 >>= 1;
        uPower2++;
    }
    return u32 == 1 ? uPower2 : 0;
}

/**
 * Sets the L1, L2 and offset bitmasks and L1 and L2 bit shift members.
 *
 * @returns nothing.
 * @param   pImage    The image instance data.
 */
static void qedTableMasksInit(PQEDIMAGE pImage)
{
    uint32_t cClusterBits, cTableBits;

    cClusterBits = qedGetPowerOfTwo(pImage->cbCluster);
    cTableBits   = qedGetPowerOfTwo(pImage->cTableEntries);

    Assert(cClusterBits + 2 * cTableBits <= 64);

    pImage->fOffsetMask = ((uint64_t)pImage->cbCluster - 1);
    pImage->fL2Mask     = ((uint64_t)pImage->cTableEntries - 1) << cClusterBits;
    pImage->cL2Shift    = cClusterBits;
    pImage->fL1Mask     = ((uint64_t)pImage->cTableEntries - 1) << (cClusterBits + cTableBits);
    pImage->cL1Shift    = cClusterBits + cTableBits;
}

/**
 * Converts a given logical offset into the
 *
 * @returns nothing.
 * @param   pImage         The image instance data.
 * @param   off            The logical offset to convert.
 * @param   pidxL1         Where to store the index in the L1 table on success.
 * @param   pidxL2         Where to store the index in the L2 table on success.
 * @param   poffCluster    Where to store the offset in the cluster on success.
 */
DECLINLINE(void) qedConvertLogicalOffset(PQEDIMAGE pImage, uint64_t off, uint32_t *pidxL1,
                                         uint32_t *pidxL2, uint32_t *poffCluster)
{
    AssertPtr(pidxL1);
    AssertPtr(pidxL2);
    AssertPtr(poffCluster);

    *poffCluster = off & pImage->fOffsetMask;
    *pidxL1      = (off & pImage->fL1Mask) >> pImage->cL1Shift;
    *pidxL2      = (off & pImage->fL2Mask) >> pImage->cL2Shift;
}

/**
 * Converts Cluster size to a byte size.
 *
 * @returns Number of bytes derived from the given number of clusters.
 * @param   pImage    The image instance data.
 * @param   cClusters The clusters to convert.
 */
DECLINLINE(uint64_t) qedCluster2Byte(PQEDIMAGE pImage, uint64_t cClusters)
{
    return cClusters * pImage->cbCluster;
}

/**
 * Converts number of bytes to cluster size rounding to the next cluster.
 *
 * @returns Number of bytes derived from the given number of clusters.
 * @param   pImage    The image instance data.
 * @param   cb        Number of bytes to convert.
 */
DECLINLINE(uint64_t) qedByte2Cluster(PQEDIMAGE pImage, uint64_t cb)
{
    return cb / pImage->cbCluster + (cb % pImage->cbCluster ? 1 : 0);
}

/**
 * Allocates a new cluster in the image.
 *
 * @returns The start offset of the new cluster in the image.
 * @param   pImage    The image instance data.
 * @param   cCLusters Number of clusters to allocate.
 */
DECLINLINE(uint64_t) qedClusterAllocate(PQEDIMAGE pImage, uint32_t cClusters)
{
    uint64_t offCluster;

    offCluster = pImage->cbImage;
    pImage->cbImage += cClusters*pImage->cbCluster;

    return offCluster;
}

/**
 * Returns the real image offset for a given cluster or an error if the cluster is not
 * yet allocated.
 *
 * @returns VBox status code.
 *          VERR_VD_BLOCK_FREE if the cluster is not yet allocated.
 * @param   pImage        The image instance data.
 * @param   pIoCtx        The I/O context.
 * @param   idxL1         The L1 index.
 * @param   idxL2         The L2 index.
 * @param   offCluster    Offset inside the cluster.
 * @param   poffImage     Where to store the image offset on success;
 */
static int qedConvertToImageOffset(PQEDIMAGE pImage, PVDIOCTX pIoCtx,
                                   uint32_t idxL1, uint32_t idxL2,
                                   uint32_t offCluster, uint64_t *poffImage)
{
    int rc = VERR_VD_BLOCK_FREE;

    AssertReturn(idxL1 < pImage->cTableEntries, VERR_INVALID_PARAMETER);
    AssertReturn(idxL2 < pImage->cTableEntries, VERR_INVALID_PARAMETER);

    if (pImage->paL1Table[idxL1])
    {
        PQEDL2CACHEENTRY pL2Entry;

        rc = qedL2TblCacheFetchAsync(pImage, pIoCtx, pImage->paL1Table[idxL1],
                                     &pL2Entry);
        if (RT_SUCCESS(rc))
        {
            /* Get real file offset. */
            if (pL2Entry->paL2Tbl[idxL2])
                *poffImage = pL2Entry->paL2Tbl[idxL2] + offCluster;
            else
                rc = VERR_VD_BLOCK_FREE;

            qedL2TblCacheEntryRelease(pL2Entry);
        }
    }

    return rc;
}


/**
 * Internal. Flush image data to disk.
 */
static int qedFlushImage(PQEDIMAGE pImage)
{
    int rc = VINF_SUCCESS;

    if (   pImage->pStorage
        && !(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
    {
        QedHeader Header;

        Assert(!(pImage->cbTable % pImage->cbCluster));
#if defined(RT_BIG_ENDIAN)
        uint64_t *paL1TblImg = (uint64_t *)RTMemAllocZ(pImage->cbTable);
        if (paL1TblImg)
        {
            qedTableConvertFromHostEndianess(paL1TblImg, pImage->paL1Table,
                                             pImage->cTableEntries);
            rc = vdIfIoIntFileWriteSync(pImage->pIfIo, pImage->pStorage,
                                        pImage->offL1Table, paL1TblImg,
                                        pImage->cbTable, NULL);
            RTMemFree(paL1TblImg);
        }
        else
            rc = VERR_NO_MEMORY;
#else
        /* Write L1 table directly. */
        rc = vdIfIoIntFileWriteSync(pImage->pIfIo, pImage->pStorage, pImage->offL1Table,
                                    pImage->paL1Table, pImage->cbTable);
#endif
        if (RT_SUCCESS(rc))
        {
            /* Write header. */
            qedHdrConvertFromHostEndianess(pImage, &Header);
            rc = vdIfIoIntFileWriteSync(pImage->pIfIo, pImage->pStorage, 0, &Header,
                                        sizeof(Header));
            if (RT_SUCCESS(rc))
                rc = vdIfIoIntFileFlushSync(pImage->pIfIo, pImage->pStorage);
        }
    }

    return rc;
}

/**
 * Flush image data to disk - version for async I/O.
 *
 * @returns VBox status code.
 * @param   pImage    The image instance data.
 * @param   pIoCtx    The I/o context
 */
static int qedFlushImageAsync(PQEDIMAGE pImage, PVDIOCTX pIoCtx)
{
    int rc = VINF_SUCCESS;

    if (   pImage->pStorage
        && !(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
    {
        QedHeader Header;

        Assert(!(pImage->cbTable % pImage->cbCluster));
#if defined(RT_BIG_ENDIAN)
        uint64_t *paL1TblImg = (uint64_t *)RTMemAllocZ(pImage->cbTable);
        if (paL1TblImg)
        {
            qedTableConvertFromHostEndianess(paL1TblImg, pImage->paL1Table,
                                             pImage->cTableEntries);
            rc = vdIfIoIntFileWriteMeta(pImage->pIfIo, pImage->pStorage,
                                        pImage->offL1Table, paL1TblImg,
                                        pImage->cbTable, pIoCtx, NULL, NULL);
            RTMemFree(paL1TblImg);
        }
        else
            rc = VERR_NO_MEMORY;
#else
        /* Write L1 table directly. */
        rc = vdIfIoIntFileWriteMeta(pImage->pIfIo, pImage->pStorage,
                                    pImage->offL1Table, pImage->paL1Table,
                                    pImage->cbTable, pIoCtx, NULL, NULL);
#endif
        if (RT_SUCCESS(rc) || rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
        {
            /* Write header. */
            qedHdrConvertFromHostEndianess(pImage, &Header);
            rc = vdIfIoIntFileWriteMeta(pImage->pIfIo, pImage->pStorage,
                                        0, &Header, sizeof(Header),
                                        pIoCtx, NULL, NULL);
            if (RT_SUCCESS(rc) || rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
                rc = vdIfIoIntFileFlush(pImage->pIfIo, pImage->pStorage,
                                        pIoCtx, NULL, NULL);
        }
    }

    return rc;
}

/**
 * Checks whether the given cluster offset is valid.
 *
 * @returns Whether the given cluster offset is valid.
 * @param   offCluster    The table offset to check.
 * @param   cbFile        The real file size of the image.
 * @param   cbCluster     The cluster size in bytes.
 */
DECLINLINE(bool) qedIsClusterOffsetValid(uint64_t offCluster, uint64_t cbFile, size_t cbCluster)
{
    return    (offCluster <= cbFile - cbCluster)
           && !(offCluster & (cbCluster - 1));
}

/**
 * Checks whether the given table offset is valid.
 *
 * @returns Whether the given table offset is valid.
 * @param   offTbl    The table offset to check.
 * @param   cbFile    The real file size of the image.
 * @param   cbTable   The table size in bytes.
 * @param   cbCluster The cluster size in bytes.
 */
DECLINLINE(bool) qedIsTblOffsetValid(uint64_t offTbl, uint64_t cbFile, size_t cbTable, size_t cbCluster)
{
    return    (offTbl <= cbFile - cbTable)
           && !(offTbl & (cbCluster - 1));
}

/**
 * Sets the specified range in the cluster bitmap checking whether any of the clusters is already
 * used before.
 *
 * @returns Whether the range was clear and is set now.
 * @param   pvClusterBitmap    The cluster bitmap to use.
 * @param   offClusterStart    The first cluster to check and set.
 * @param   offClusterEnd      The first cluster to not check and set anymore.
 */
static bool qedClusterBitmapCheckAndSet(void *pvClusterBitmap, uint32_t offClusterStart, uint32_t offClusterEnd)
{
    for (uint32_t offCluster = offClusterStart; offCluster < offClusterEnd; offCluster++)
        if (ASMBitTest(pvClusterBitmap, offCluster))
            return false;

    ASMBitSetRange(pvClusterBitmap, offClusterStart, offClusterEnd);
    return true;
}

/**
 * Checks the given image for consistency, usually called when the
 * QED_FEATURE_NEED_CHECK bit is set.
 *
 * @returns VBox status code.
 * @retval VINF_SUCCESS when the image can be accessed.
 * @param   pImage    The image instance data.
 * @param   pHeader   The header to use for checking.
 *
 * @note It is not required that the image state is fully initialized  Only
 *       The I/O interface and storage handle need to be valid.
 * @note The header must be converted to the host CPU endian format already
 *       and should be validated already.
 */
static int qedCheckImage(PQEDIMAGE pImage, PQedHeader pHeader)
{
    uint64_t cbFile;
    uint32_t cbTable;
    uint32_t cTableEntries;
    uint64_t *paL1Tbl = NULL;
    uint64_t *paL2Tbl = NULL;
    void *pvClusterBitmap = NULL;
    uint32_t offClusterStart;
    int rc = VINF_SUCCESS;

    pImage->cbCluster = pHeader->u32ClusterSize;
    cbTable       = pHeader->u32TableSize * pHeader->u32ClusterSize;
    cTableEntries = cbTable / sizeof(uint64_t);

    do
    {
        rc = vdIfIoIntFileGetSize(pImage->pIfIo, pImage->pStorage, &cbFile);
        if (RT_FAILURE(rc))
        {
            rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS,
                           N_("Qed: Querying the file size of image '%s' failed"),
                           pImage->pszFilename);
            break;
        }

        /* Allocate L1 table. */
        paL1Tbl = (uint64_t *)RTMemAllocZ(cbTable);
        if (!paL1Tbl)
        {
            rc = vdIfError(pImage->pIfError, VERR_NO_MEMORY, RT_SRC_POS,
                          N_("Qed: Allocating memory for the L1 table for image '%s' failed"),
                          pImage->pszFilename);
            break;
        }

        paL2Tbl = (uint64_t *)RTMemAllocZ(cbTable);
        if (!paL2Tbl)
        {
            rc = vdIfError(pImage->pIfError, VERR_NO_MEMORY, RT_SRC_POS,
                           N_("Qed: Allocating memory for the L2 table for image '%s' failed"),
                           pImage->pszFilename);
            break;
        }

        pvClusterBitmap = RTMemAllocZ(cbFile / pHeader->u32ClusterSize / 8);
        if (!pvClusterBitmap)
        {
            rc = vdIfError(pImage->pIfError, VERR_NO_MEMORY, RT_SRC_POS,
                           N_("Qed: Allocating memory for the cluster bitmap for image '%s' failed"),
                           pImage->pszFilename);
            break;
        }

        /* Validate L1 table offset. */
        if (!qedIsTblOffsetValid(pHeader->u64OffL1Table, cbFile, cbTable, pHeader->u32ClusterSize))
        {
            rc = vdIfError(pImage->pIfError, VERR_VD_GEN_INVALID_HEADER, RT_SRC_POS,
                           N_("Qed: L1 table offset of image '%s' is corrupt (%llu)"),
                           pImage->pszFilename, pHeader->u64OffL1Table);
            break;
        }

        /* Read L1 table. */
        rc = vdIfIoIntFileReadSync(pImage->pIfIo, pImage->pStorage,
                                   pHeader->u64OffL1Table, paL1Tbl, cbTable);
        if (RT_FAILURE(rc))
        {
            rc = vdIfError(pImage->pIfError, VERR_VD_GEN_INVALID_HEADER, RT_SRC_POS,
                           N_("Qed: Reading the L1 table from image '%s' failed"),
                           pImage->pszFilename);
            break;
        }

        /* Mark the L1 table in cluster bitmap. */
        ASMBitSet(pvClusterBitmap, 0); /* Header is always in cluster 0. */
        offClusterStart = qedByte2Cluster(pImage, pHeader->u64OffL1Table);
        bool fSet = qedClusterBitmapCheckAndSet(pvClusterBitmap, offClusterStart, offClusterStart + pHeader->u32TableSize);
        Assert(fSet);

        /* Scan the L1 and L2 tables for invalid entries. */
        qedTableConvertToHostEndianess(paL1Tbl, cTableEntries);

        for (unsigned iL1 = 0; iL1 < cTableEntries; iL1++)
        {
            if (!paL1Tbl[iL1])
                continue; /* Skip unallocated clusters. */

            if (!qedIsTblOffsetValid(paL1Tbl[iL1], cbFile, cbTable, pHeader->u32ClusterSize))
            {
                rc = vdIfError(pImage->pIfError, VERR_VD_GEN_INVALID_HEADER, RT_SRC_POS,
                               N_("Qed: Entry %d of the L1 table from image '%s' is invalid (%llu)"),
                               iL1, pImage->pszFilename, paL1Tbl[iL1]);
                break;
            }

            /* Now check that the clusters are not allocated already. */
            offClusterStart = qedByte2Cluster(pImage, paL1Tbl[iL1]);
            fSet = qedClusterBitmapCheckAndSet(pvClusterBitmap, offClusterStart, offClusterStart + pHeader->u32TableSize);
            if (!fSet)
            {
                rc = vdIfError(pImage->pIfError, VERR_VD_GEN_INVALID_HEADER, RT_SRC_POS,
                               N_("Qed: Entry %d of the L1 table from image '%s' points to a already used cluster (%llu)"),
                               iL1, pImage->pszFilename, paL1Tbl[iL1]);
                break;
            }

            /* Read the linked L2 table and check it. */
            rc = vdIfIoIntFileReadSync(pImage->pIfIo, pImage->pStorage,
                                       paL1Tbl[iL1], paL2Tbl, cbTable);
            if (RT_FAILURE(rc))
            {
                rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS,
                               N_("Qed: Reading the L2 table from image '%s' failed"),
                               pImage->pszFilename);
                break;
            }

            /* Check all L2 entries. */
            for (unsigned iL2 = 0; iL2 < cTableEntries; iL2++)
            {
                if (paL2Tbl[iL2])
                    continue; /* Skip unallocated clusters. */

                if (!qedIsClusterOffsetValid(paL2Tbl[iL2], cbFile, pHeader->u32ClusterSize))
                {
                    rc = vdIfError(pImage->pIfError, VERR_VD_GEN_INVALID_HEADER, RT_SRC_POS,
                                   N_("Qed: Entry %d of the L2 table from image '%s' is invalid (%llu)"),
                                   iL2, pImage->pszFilename, paL2Tbl[iL2]);
                    break;
                }

                /* Now check that the clusters are not allocated already. */
                offClusterStart = qedByte2Cluster(pImage, paL2Tbl[iL2]);
                fSet = qedClusterBitmapCheckAndSet(pvClusterBitmap, offClusterStart, offClusterStart + 1);
                if (!fSet)
                {
                    rc = vdIfError(pImage->pIfError, VERR_VD_GEN_INVALID_HEADER, RT_SRC_POS,
                                   N_("Qed: Entry %d of the L2 table from image '%s' points to a already used cluster (%llu)"),
                                   iL2, pImage->pszFilename, paL2Tbl[iL2]);
                    break;
                }
            }
        }
    } while(0);

    if (paL1Tbl)
        RTMemFree(paL1Tbl);
    if (paL2Tbl)
        RTMemFree(paL2Tbl);
    if (pvClusterBitmap)
        RTMemFree(pvClusterBitmap);

    return rc;
}

/**
 * Internal. Free all allocated space for representing an image except pImage,
 * and optionally delete the image from disk.
 */
static int qedFreeImage(PQEDIMAGE pImage, bool fDelete)
{
    int rc = VINF_SUCCESS;

    /* Freeing a never allocated image (e.g. because the open failed) is
     * not signalled as an error. After all nothing bad happens. */
    if (pImage)
    {
        if (pImage->pStorage)
        {
            /* No point updating the file that is deleted anyway. */
            if (!fDelete)
                qedFlushImage(pImage);

            rc = vdIfIoIntFileClose(pImage->pIfIo, pImage->pStorage);
            pImage->pStorage = NULL;
        }

        if (pImage->paL1Table)
            RTMemFree(pImage->paL1Table);

        if (pImage->pszBackingFilename)
        {
            RTMemFree(pImage->pszBackingFilename);
            pImage->pszBackingFilename = NULL;
        }

        qedL2TblCacheDestroy(pImage);

        if (fDelete && pImage->pszFilename)
            vdIfIoIntFileDelete(pImage->pIfIo, pImage->pszFilename);
    }

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/**
 * Internal: Open an image, constructing all necessary data structures.
 */
static int qedOpenImage(PQEDIMAGE pImage, unsigned uOpenFlags)
{
    int rc;

    pImage->uOpenFlags = uOpenFlags;

    pImage->pIfError = VDIfErrorGet(pImage->pVDIfsDisk);
    pImage->pIfIo = VDIfIoIntGet(pImage->pVDIfsImage);
    AssertPtrReturn(pImage->pIfIo, VERR_INVALID_PARAMETER);

    /*
     * Open the image.
     */
    rc = vdIfIoIntFileOpen(pImage->pIfIo, pImage->pszFilename,
                           VDOpenFlagsToFileOpenFlags(uOpenFlags,
                                                      false /* fCreate */),
                           &pImage->pStorage);
    if (RT_FAILURE(rc))
    {
        /* Do NOT signal an appropriate error here, as the VD layer has the
         * choice of retrying the open if it failed. */
        goto out;
    }

    uint64_t cbFile;
    QedHeader Header;
    rc = vdIfIoIntFileGetSize(pImage->pIfIo, pImage->pStorage, &cbFile);
    if (RT_FAILURE(rc))
        goto out;
    if (cbFile > sizeof(Header))
    {
        rc = vdIfIoIntFileReadSync(pImage->pIfIo, pImage->pStorage, 0, &Header, sizeof(Header));
        if (   RT_SUCCESS(rc)
            && qedHdrConvertToHostEndianess(&Header))
        {
            if (   !(Header.u64FeatureFlags & ~QED_FEATURE_MASK)
                && !(Header.u64FeatureFlags & QED_FEATURE_BACKING_FILE_NO_PROBE))
            {
                if (Header.u64FeatureFlags & QED_FEATURE_NEED_CHECK)
                {
                    /* Image needs checking. */
                    if (!(uOpenFlags & VD_OPEN_FLAGS_READONLY))
                        rc = qedCheckImage(pImage, &Header);
                    else
                        rc = vdIfError(pImage->pIfError, VERR_NOT_SUPPORTED, RT_SRC_POS,
                                       N_("Qed: Image '%s' needs checking but is opened readonly"),
                                       pImage->pszFilename);
                }

                if (   RT_SUCCESS(rc)
                    && (Header.u64FeatureFlags & QED_FEATURE_BACKING_FILE))
                {
                    /* Load backing filename from image. */
                    pImage->pszBackingFilename = (char *)RTMemAllocZ(Header.u32BackingFilenameSize + 1); /* +1 for \0 terminator. */
                    if (pImage->pszBackingFilename)
                    {
                        pImage->cbBackingFilename  = Header.u32BackingFilenameSize;
                        pImage->offBackingFilename = Header.u32OffBackingFilename;
                        rc = vdIfIoIntFileReadSync(pImage->pIfIo, pImage->pStorage,
                                                   Header.u32OffBackingFilename, pImage->pszBackingFilename,
                                                   Header.u32BackingFilenameSize);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                }

                if (RT_SUCCESS(rc))
                {
                    pImage->cbImage       = cbFile;
                    pImage->cbCluster     = Header.u32ClusterSize;
                    pImage->cbTable       = Header.u32TableSize * pImage->cbCluster;
                    pImage->cTableEntries = pImage->cbTable / sizeof(uint64_t);
                    pImage->offL1Table    = Header.u64OffL1Table;
                    pImage->cbSize        = Header.u64Size;
                    qedTableMasksInit(pImage);

                    /* Allocate L1 table. */
                    pImage->paL1Table     = (uint64_t *)RTMemAllocZ(pImage->cbTable);
                    if (pImage->paL1Table)
                    {
                        /* Read from the image. */
                        rc = vdIfIoIntFileReadSync(pImage->pIfIo, pImage->pStorage,
                                                   pImage->offL1Table, pImage->paL1Table,
                                                   pImage->cbTable);
                        if (RT_SUCCESS(rc))
                        {
                            qedTableConvertToHostEndianess(pImage->paL1Table, pImage->cTableEntries);
                            rc = qedL2TblCacheCreate(pImage);
                            if (RT_SUCCESS(rc))
                            {
                                /* If the consistency check succeeded, clear the flag by flushing the image. */
                                if (Header.u64FeatureFlags & QED_FEATURE_NEED_CHECK)
                                    rc = qedFlushImage(pImage);
                            }
                            else
                                rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS,
                                               N_("Qed: Creating the L2 table cache for image '%s' failed"),
                                               pImage->pszFilename);
                        }
                        else
                            rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS,
                                           N_("Qed: Reading the L1 table for image '%s' failed"),
                                           pImage->pszFilename);
                    }
                    else
                        rc = vdIfError(pImage->pIfError, VERR_NO_MEMORY, RT_SRC_POS,
                                       N_("Qed: Out of memory allocating L1 table for image '%s'"),
                                       pImage->pszFilename);
                }
            }
            else
                rc = vdIfError(pImage->pIfError, VERR_NOT_SUPPORTED, RT_SRC_POS,
                               N_("Qed: The image '%s' makes use of unsupported features"),
                               pImage->pszFilename);
        }
        else if (RT_SUCCESS(rc))
            rc = VERR_VD_GEN_INVALID_HEADER;
    }
    else
        rc = VERR_VD_GEN_INVALID_HEADER;

out:
    if (RT_FAILURE(rc))
        qedFreeImage(pImage, false);
    return rc;
}

/**
 * Internal: Create a qed image.
 */
static int qedCreateImage(PQEDIMAGE pImage, uint64_t cbSize,
                          unsigned uImageFlags, const char *pszComment,
                          PCVDGEOMETRY pPCHSGeometry,
                          PCVDGEOMETRY pLCHSGeometry, unsigned uOpenFlags,
                          PFNVDPROGRESS pfnProgress, void *pvUser,
                          unsigned uPercentStart, unsigned uPercentSpan)
{
    int rc;
    int32_t fOpen;

    if (uImageFlags & VD_IMAGE_FLAGS_FIXED)
    {
        rc = vdIfError(pImage->pIfError, VERR_VD_INVALID_TYPE, RT_SRC_POS, N_("Qed: cannot create fixed image '%s'"), pImage->pszFilename);
        goto out;
    }

    pImage->uOpenFlags   = uOpenFlags & ~VD_OPEN_FLAGS_READONLY;
    pImage->uImageFlags  = uImageFlags;
    pImage->PCHSGeometry = *pPCHSGeometry;
    pImage->LCHSGeometry = *pLCHSGeometry;

    pImage->pIfError = VDIfErrorGet(pImage->pVDIfsDisk);
    pImage->pIfIo = VDIfIoIntGet(pImage->pVDIfsImage);
    AssertPtrReturn(pImage->pIfIo, VERR_INVALID_PARAMETER);

    /* Create image file. */
    fOpen = VDOpenFlagsToFileOpenFlags(pImage->uOpenFlags, true /* fCreate */);
    rc = vdIfIoIntFileOpen(pImage->pIfIo, pImage->pszFilename, fOpen, &pImage->pStorage);
    if (RT_FAILURE(rc))
    {
        rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS, N_("Qed: cannot create image '%s'"), pImage->pszFilename);
        goto out;
    }

    /* Init image state. */
    pImage->cbSize             = cbSize;
    pImage->cbCluster          = QED_CLUSTER_SIZE_DEFAULT;
    pImage->cbTable            = qedCluster2Byte(pImage, QED_TABLE_SIZE_DEFAULT);
    pImage->cTableEntries      = pImage->cbTable / sizeof(uint64_t);
    pImage->offL1Table         = qedCluster2Byte(pImage, 1); /* Cluster 0 is the header. */
    pImage->cbImage            = (1 * pImage->cbCluster) + pImage->cbTable; /* Header + L1 table size. */
    pImage->cbBackingFilename  = 0;
    pImage->offBackingFilename = 0;
    qedTableMasksInit(pImage);

    /* Init L1 table. */
    pImage->paL1Table     = (uint64_t *)RTMemAllocZ(pImage->cbTable);
    if (!pImage->paL1Table)
    {
        rc = vdIfError(pImage->pIfError, VERR_NO_MEMORY, RT_SRC_POS, N_("Qed: cannot allocate memory for L1 table of image '%s'"),
                       pImage->pszFilename);
        goto out;
    }

    rc = qedL2TblCacheCreate(pImage);
    if (RT_FAILURE(rc))
    {
        rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS, N_("Qed: Failed to create L2 cache for image '%s'"),
                       pImage->pszFilename);
        goto out;
    }

    if (RT_SUCCESS(rc) && pfnProgress)
        pfnProgress(pvUser, uPercentStart + uPercentSpan * 98 / 100);

    rc = qedFlushImage(pImage);

out:
    if (RT_SUCCESS(rc) && pfnProgress)
        pfnProgress(pvUser, uPercentStart + uPercentSpan);

    if (RT_FAILURE(rc))
        qedFreeImage(pImage, rc != VERR_ALREADY_EXISTS);
    return rc;
}

/**
 * Rollback anything done during async cluster allocation.
 *
 * @returns VBox status code.
 * @param   pImage           The image instance data.
 * @param   pIoCtx           The I/O context.
 * @param   pClusterAlloc    The cluster allocation to rollback.
 */
static int qedAsyncClusterAllocRollback(PQEDIMAGE pImage, PVDIOCTX pIoCtx, PQEDCLUSTERASYNCALLOC pClusterAlloc)
{
    int rc = VINF_SUCCESS;

    switch (pClusterAlloc->enmAllocState)
    {
        case QEDCLUSTERASYNCALLOCSTATE_L2_ALLOC:
        case QEDCLUSTERASYNCALLOCSTATE_L2_LINK:
        {
            /* Assumption right now is that the L1 table is not modified if the link fails. */
            rc = vdIfIoIntFileSetSize(pImage->pIfIo, pImage->pStorage, pClusterAlloc->cbImageOld);
            qedL2TblCacheEntryRelease(pClusterAlloc->pL2Entry); /* Release L2 cache entry. */
            qedL2TblCacheEntryFree(pImage, pClusterAlloc->pL2Entry); /* Free it, it is not in the cache yet. */
            break;
        }
        case QEDCLUSTERASYNCALLOCSTATE_USER_ALLOC:
        case QEDCLUSTERASYNCALLOCSTATE_USER_LINK:
        {
            /* Assumption right now is that the L2 table is not modified if the link fails. */
            rc = vdIfIoIntFileSetSize(pImage->pIfIo, pImage->pStorage, pClusterAlloc->cbImageOld);
            qedL2TblCacheEntryRelease(pClusterAlloc->pL2Entry); /* Release L2 cache entry. */
            break;
        }
        default:
            AssertMsgFailed(("Invalid cluster allocation state %d\n", pClusterAlloc->enmAllocState));
            rc = VERR_INVALID_STATE;
    }

    RTMemFree(pClusterAlloc);
    return rc;
}

/**
 * Updates the state of the async cluster allocation.
 *
 * @returns VBox status code.
 * @param   pBackendData    The opaque backend data.
 * @param   pIoCtx          I/O context associated with this request.
 * @param   pvUser          Opaque user data passed during a read/write request.
 * @param   rcReq           Status code for the completed request.
 */
static DECLCALLBACK(int) qedAsyncClusterAllocUpdate(void *pBackendData, PVDIOCTX pIoCtx, void *pvUser, int rcReq)
{
    int rc = VINF_SUCCESS;
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    PQEDCLUSTERASYNCALLOC pClusterAlloc = (PQEDCLUSTERASYNCALLOC)pvUser;

    if (RT_FAILURE(rcReq))
        return qedAsyncClusterAllocRollback(pImage, pIoCtx, pClusterAlloc);

    AssertPtr(pClusterAlloc->pL2Entry);

    switch (pClusterAlloc->enmAllocState)
    {
        case QEDCLUSTERASYNCALLOCSTATE_L2_ALLOC:
        {
            uint64_t offUpdateLe = RT_H2LE_U64(pClusterAlloc->pL2Entry->offL2Tbl);

            /* Update the link in the on disk L1 table now. */
            pClusterAlloc->enmAllocState = QEDCLUSTERASYNCALLOCSTATE_L2_LINK;
            rc = vdIfIoIntFileWriteMeta(pImage->pIfIo, pImage->pStorage,
                                        pImage->offL1Table + pClusterAlloc->idxL1*sizeof(uint64_t),
                                        &offUpdateLe, sizeof(uint64_t), pIoCtx,
                                        qedAsyncClusterAllocUpdate, pClusterAlloc);
            if (rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
                break;
            else if (RT_FAILURE(rc))
            {
                /* Rollback. */
                qedAsyncClusterAllocRollback(pImage, pIoCtx, pClusterAlloc);
                break;
            }
            /* Success, fall through. */
        }
        case QEDCLUSTERASYNCALLOCSTATE_L2_LINK:
        {
            /* L2 link updated in L1 , save L2 entry in cache and allocate new user data cluster. */
            uint64_t offData = qedClusterAllocate(pImage, 1);

            /* Update the link in the in memory L1 table now. */
            pImage->paL1Table[pClusterAlloc->idxL1] = pClusterAlloc->pL2Entry->offL2Tbl;
            qedL2TblCacheEntryInsert(pImage, pClusterAlloc->pL2Entry);

            pClusterAlloc->enmAllocState = QEDCLUSTERASYNCALLOCSTATE_USER_ALLOC;
            pClusterAlloc->cbImageOld    = offData;
            pClusterAlloc->offClusterNew = offData;

            /* Write data. */
            rc = vdIfIoIntFileWriteUser(pImage->pIfIo, pImage->pStorage,
                                        offData, pIoCtx, pClusterAlloc->cbToWrite,
                                        qedAsyncClusterAllocUpdate, pClusterAlloc);
            if (rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
                break;
            else if (RT_FAILURE(rc))
            {
                qedAsyncClusterAllocRollback(pImage, pIoCtx, pClusterAlloc);
                RTMemFree(pClusterAlloc);
                break;
            }
        }
        case QEDCLUSTERASYNCALLOCSTATE_USER_ALLOC:
        {
            uint64_t offUpdateLe = RT_H2LE_U64(pClusterAlloc->offClusterNew);

            pClusterAlloc->enmAllocState = QEDCLUSTERASYNCALLOCSTATE_USER_LINK;

            /* Link L2 table and update it. */
            rc = vdIfIoIntFileWriteMeta(pImage->pIfIo, pImage->pStorage,
                                        pImage->paL1Table[pClusterAlloc->idxL1] + pClusterAlloc->idxL2*sizeof(uint64_t),
                                        &offUpdateLe, sizeof(uint64_t), pIoCtx,
                                        qedAsyncClusterAllocUpdate, pClusterAlloc);
            if (rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
                break;
            else if (RT_FAILURE(rc))
            {
                qedAsyncClusterAllocRollback(pImage, pIoCtx, pClusterAlloc);
                RTMemFree(pClusterAlloc);
                break;
            }
        }
        case QEDCLUSTERASYNCALLOCSTATE_USER_LINK:
        {
            /* Everything done without errors, signal completion. */
            pClusterAlloc->pL2Entry->paL2Tbl[pClusterAlloc->idxL2] = pClusterAlloc->offClusterNew;
            qedL2TblCacheEntryRelease(pClusterAlloc->pL2Entry);
            RTMemFree(pClusterAlloc);
            rc = VINF_SUCCESS;
            break;
        }
        default:
            AssertMsgFailed(("Invalid async cluster allocation state %d\n",
                             pClusterAlloc->enmAllocState));
    }

    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnCheckIfValid */
static int qedCheckIfValid(const char *pszFilename, PVDINTERFACE pVDIfsDisk,
                           PVDINTERFACE pVDIfsImage, VDTYPE *penmType)
{
    LogFlowFunc(("pszFilename=\"%s\" pVDIfsDisk=%#p pVDIfsImage=%#p\n", pszFilename, pVDIfsDisk, pVDIfsImage));
    PVDIOSTORAGE pStorage = NULL;
    uint64_t cbFile;
    int rc = VINF_SUCCESS;

    /* Get I/O interface. */
    PVDINTERFACEIOINT pIfIo = VDIfIoIntGet(pVDIfsImage);
    AssertPtrReturn(pIfIo, VERR_INVALID_PARAMETER);

    if (   !VALID_PTR(pszFilename)
        || !*pszFilename)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /*
     * Open the file and read the footer.
     */
    rc = vdIfIoIntFileOpen(pIfIo, pszFilename,
                           VDOpenFlagsToFileOpenFlags(VD_OPEN_FLAGS_READONLY,
                                                      false /* fCreate */),
                           &pStorage);
    if (RT_SUCCESS(rc))
        rc = vdIfIoIntFileGetSize(pIfIo, pStorage, &cbFile);

    if (   RT_SUCCESS(rc)
        && cbFile > sizeof(QedHeader))
    {
        QedHeader Header;

        rc = vdIfIoIntFileReadSync(pIfIo, pStorage, 0, &Header, sizeof(Header));
        if (   RT_SUCCESS(rc)
            && qedHdrConvertToHostEndianess(&Header))
        {
            *penmType = VDTYPE_HDD;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_VD_GEN_INVALID_HEADER;
    }
    else
        rc = VERR_VD_GEN_INVALID_HEADER;

    if (pStorage)
        vdIfIoIntFileClose(pIfIo, pStorage);

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnOpen */
static int qedOpen(const char *pszFilename, unsigned uOpenFlags,
                   PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                   VDTYPE enmType, void **ppBackendData)
{
    LogFlowFunc(("pszFilename=\"%s\" uOpenFlags=%#x pVDIfsDisk=%#p pVDIfsImage=%#p ppBackendData=%#p\n", pszFilename, uOpenFlags, pVDIfsDisk, pVDIfsImage, ppBackendData));
    int rc;
    PQEDIMAGE pImage;

    /* Check open flags. All valid flags are supported. */
    if (uOpenFlags & ~VD_OPEN_FLAGS_MASK)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Check remaining arguments. */
    if (   !VALID_PTR(pszFilename)
        || !*pszFilename)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }


    pImage = (PQEDIMAGE)RTMemAllocZ(sizeof(QEDIMAGE));
    if (!pImage)
    {
        rc = VERR_NO_MEMORY;
        goto out;
    }
    pImage->pszFilename = pszFilename;
    pImage->pStorage = NULL;
    pImage->pVDIfsDisk = pVDIfsDisk;
    pImage->pVDIfsImage = pVDIfsImage;

    rc = qedOpenImage(pImage, uOpenFlags);
    if (RT_SUCCESS(rc))
        *ppBackendData = pImage;
    else
        RTMemFree(pImage);

out:
    LogFlowFunc(("returns %Rrc (pBackendData=%#p)\n", rc, *ppBackendData));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnCreate */
static int qedCreate(const char *pszFilename, uint64_t cbSize,
                     unsigned uImageFlags, const char *pszComment,
                     PCVDGEOMETRY pPCHSGeometry, PCVDGEOMETRY pLCHSGeometry,
                     PCRTUUID pUuid, unsigned uOpenFlags,
                     unsigned uPercentStart, unsigned uPercentSpan,
                     PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                     PVDINTERFACE pVDIfsOperation, void **ppBackendData)
{
    LogFlowFunc(("pszFilename=\"%s\" cbSize=%llu uImageFlags=%#x pszComment=\"%s\" pPCHSGeometry=%#p pLCHSGeometry=%#p Uuid=%RTuuid uOpenFlags=%#x uPercentStart=%u uPercentSpan=%u pVDIfsDisk=%#p pVDIfsImage=%#p pVDIfsOperation=%#p ppBackendData=%#p",
                 pszFilename, cbSize, uImageFlags, pszComment, pPCHSGeometry, pLCHSGeometry, pUuid, uOpenFlags, uPercentStart, uPercentSpan, pVDIfsDisk, pVDIfsImage, pVDIfsOperation, ppBackendData));
    int rc;
    PQEDIMAGE pImage;

    PFNVDPROGRESS pfnProgress = NULL;
    void *pvUser = NULL;
    PVDINTERFACEPROGRESS pIfProgress = VDIfProgressGet(pVDIfsOperation);
    if (pIfProgress)
    {
        pfnProgress = pIfProgress->pfnProgress;
        pvUser = pIfProgress->Core.pvUser;
    }

    /* Check open flags. All valid flags are supported. */
    if (uOpenFlags & ~VD_OPEN_FLAGS_MASK)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Check remaining arguments. */
    if (   !VALID_PTR(pszFilename)
        || !*pszFilename
        || !VALID_PTR(pPCHSGeometry)
        || !VALID_PTR(pLCHSGeometry))
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    pImage = (PQEDIMAGE)RTMemAllocZ(sizeof(QEDIMAGE));
    if (!pImage)
    {
        rc = VERR_NO_MEMORY;
        goto out;
    }
    pImage->pszFilename = pszFilename;
    pImage->pStorage = NULL;
    pImage->pVDIfsDisk = pVDIfsDisk;
    pImage->pVDIfsImage = pVDIfsImage;

    rc = qedCreateImage(pImage, cbSize, uImageFlags, pszComment,
                        pPCHSGeometry, pLCHSGeometry, uOpenFlags,
                        pfnProgress, pvUser, uPercentStart, uPercentSpan);
    if (RT_SUCCESS(rc))
    {
        /* So far the image is opened in read/write mode. Make sure the
         * image is opened in read-only mode if the caller requested that. */
        if (uOpenFlags & VD_OPEN_FLAGS_READONLY)
        {
            qedFreeImage(pImage, false);
            rc = qedOpenImage(pImage, uOpenFlags);
            if (RT_FAILURE(rc))
            {
                RTMemFree(pImage);
                goto out;
            }
        }
        *ppBackendData = pImage;
    }
    else
        RTMemFree(pImage);

out:
    LogFlowFunc(("returns %Rrc (pBackendData=%#p)\n", rc, *ppBackendData));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnRename */
static int qedRename(void *pBackendData, const char *pszFilename)
{
    LogFlowFunc(("pBackendData=%#p pszFilename=%#p\n", pBackendData, pszFilename));
    int rc = VINF_SUCCESS;
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;

    /* Check arguments. */
    if (   !pImage
        || !pszFilename
        || !*pszFilename)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Close the image. */
    rc = qedFreeImage(pImage, false);
    if (RT_FAILURE(rc))
        goto out;

    /* Rename the file. */
    rc = vdIfIoIntFileMove(pImage->pIfIo, pImage->pszFilename, pszFilename, 0);
    if (RT_FAILURE(rc))
    {
        /* The move failed, try to reopen the original image. */
        int rc2 = qedOpenImage(pImage, pImage->uOpenFlags);
        if (RT_FAILURE(rc2))
            rc = rc2;

        goto out;
    }

    /* Update pImage with the new information. */
    pImage->pszFilename = pszFilename;

    /* Open the old image with new name. */
    rc = qedOpenImage(pImage, pImage->uOpenFlags);
    if (RT_FAILURE(rc))
        goto out;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnClose */
static int qedClose(void *pBackendData, bool fDelete)
{
    LogFlowFunc(("pBackendData=%#p fDelete=%d\n", pBackendData, fDelete));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    rc = qedFreeImage(pImage, fDelete);
    RTMemFree(pImage);

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

static int qedRead(void *pBackendData, uint64_t uOffset, size_t cbToRead,
                   PVDIOCTX pIoCtx, size_t *pcbActuallyRead)
{
    LogFlowFunc(("pBackendData=%#p uOffset=%llu pIoCtx=%#p cbToRead=%zu pcbActuallyRead=%#p\n",
                 pBackendData, uOffset, pIoCtx, cbToRead, pcbActuallyRead));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    uint32_t offCluster = 0;
    uint32_t idxL1      = 0;
    uint32_t idxL2      = 0;
    uint64_t offFile    = 0;
    int rc;

    AssertPtr(pImage);
    Assert(uOffset % 512 == 0);
    Assert(cbToRead % 512 == 0);

    if (!VALID_PTR(pIoCtx) || !cbToRead)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    if (   uOffset + cbToRead > pImage->cbSize
        || cbToRead == 0)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    qedConvertLogicalOffset(pImage, uOffset, &idxL1, &idxL2, &offCluster);

    /* Clip read size to remain in the cluster. */
    cbToRead = RT_MIN(cbToRead, pImage->cbCluster - offCluster);

    /* Get offset in image. */
    rc = qedConvertToImageOffset(pImage, pIoCtx, idxL1, idxL2, offCluster, &offFile);
    if (RT_SUCCESS(rc))
        rc = vdIfIoIntFileReadUser(pImage->pIfIo, pImage->pStorage, offFile,
                                   pIoCtx, cbToRead);

    if (   (   RT_SUCCESS(rc)
            || rc == VERR_VD_BLOCK_FREE
            || rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
        && pcbActuallyRead)
        *pcbActuallyRead = cbToRead;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

static int qedWrite(void *pBackendData, uint64_t uOffset, size_t cbToWrite,
                    PVDIOCTX pIoCtx, size_t *pcbWriteProcess, size_t *pcbPreRead,
                    size_t *pcbPostRead, unsigned fWrite)
{
    LogFlowFunc(("pBackendData=%#p uOffset=%llu pIoCtx=%#p cbToWrite=%zu pcbWriteProcess=%#p pcbPreRead=%#p pcbPostRead=%#p\n",
                 pBackendData, uOffset, pIoCtx, cbToWrite, pcbWriteProcess, pcbPreRead, pcbPostRead));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    uint32_t offCluster = 0;
    uint32_t idxL1      = 0;
    uint32_t idxL2      = 0;
    uint64_t offImage   = 0;
    int rc = VINF_SUCCESS;

    AssertPtr(pImage);
    Assert(!(uOffset % 512));
    Assert(!(cbToWrite % 512));

    if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
    {
        rc = VERR_VD_IMAGE_READ_ONLY;
        goto out;
    }

    if (!VALID_PTR(pIoCtx) || !cbToWrite)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    if (   uOffset + cbToWrite > pImage->cbSize
        || cbToWrite == 0)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Convert offset to L1, L2 index and cluster offset. */
    qedConvertLogicalOffset(pImage, uOffset, &idxL1, &idxL2, &offCluster);

    /* Clip write size to remain in the cluster. */
    cbToWrite = RT_MIN(cbToWrite, pImage->cbCluster - offCluster);
    Assert(!(cbToWrite % 512));

    /* Get offset in image. */
    rc = qedConvertToImageOffset(pImage, pIoCtx, idxL1, idxL2, offCluster, &offImage);
    if (RT_SUCCESS(rc))
        rc = vdIfIoIntFileWriteUser(pImage->pIfIo, pImage->pStorage,
                                    offImage, pIoCtx, cbToWrite, NULL, NULL);
    else if (rc == VERR_VD_BLOCK_FREE)
    {
        if (   cbToWrite == pImage->cbCluster
            && !(fWrite & VD_WRITE_NO_ALLOC))
        {
            PQEDL2CACHEENTRY pL2Entry = NULL;

            /* Full cluster write to previously unallocated cluster.
             * Allocate cluster and write data. */
            Assert(!offCluster);

            do
            {
                uint64_t idxUpdateLe = 0;

                /* Check if we have to allocate a new cluster for L2 tables. */
                if (!pImage->paL1Table[idxL1])
                {
                    uint64_t offL2Tbl;
                    PQEDCLUSTERASYNCALLOC pL2ClusterAlloc = NULL;

                    /* Allocate new async cluster allocation state. */
                    pL2ClusterAlloc = (PQEDCLUSTERASYNCALLOC)RTMemAllocZ(sizeof(QEDCLUSTERASYNCALLOC));
                    if (RT_UNLIKELY(!pL2ClusterAlloc))
                    {
                        rc = VERR_NO_MEMORY;
                        break;
                    }

                    pL2Entry = qedL2TblCacheEntryAlloc(pImage);
                    if (!pL2Entry)
                    {
                        rc = VERR_NO_MEMORY;
                        RTMemFree(pL2ClusterAlloc);
                        break;
                    }

                    offL2Tbl = qedClusterAllocate(pImage, qedByte2Cluster(pImage, pImage->cbTable));
                    pL2Entry->offL2Tbl = offL2Tbl;
                    memset(pL2Entry->paL2Tbl, 0, pImage->cbTable);

                    pL2ClusterAlloc->enmAllocState = QEDCLUSTERASYNCALLOCSTATE_L2_ALLOC;
                    pL2ClusterAlloc->cbImageOld    = offL2Tbl;
                    pL2ClusterAlloc->offClusterNew = offL2Tbl;
                    pL2ClusterAlloc->idxL1         = idxL1;
                    pL2ClusterAlloc->idxL2         = idxL2;
                    pL2ClusterAlloc->cbToWrite     = cbToWrite;
                    pL2ClusterAlloc->pL2Entry      = pL2Entry;

                    /*
                     * Write the L2 table first and link to the L1 table afterwards.
                     * If something unexpected happens the worst case which can happen
                     * is a leak of some clusters.
                     */
                    rc = vdIfIoIntFileWriteMeta(pImage->pIfIo, pImage->pStorage,
                                                offL2Tbl, pL2Entry->paL2Tbl, pImage->cbTable, pIoCtx,
                                                qedAsyncClusterAllocUpdate, pL2ClusterAlloc);
                    if (rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
                        break;
                    else if (RT_FAILURE(rc))
                    {
                        RTMemFree(pL2ClusterAlloc);
                        qedL2TblCacheEntryFree(pImage, pL2Entry);
                        break;
                    }

                    rc = qedAsyncClusterAllocUpdate(pImage, pIoCtx, pL2ClusterAlloc, rc);
                }
                else
                {
                    rc = qedL2TblCacheFetchAsync(pImage, pIoCtx, pImage->paL1Table[idxL1],
                                                 &pL2Entry);

                    if (RT_SUCCESS(rc))
                    {
                        PQEDCLUSTERASYNCALLOC pDataClusterAlloc = NULL;

                        /* Allocate new async cluster allocation state. */
                        pDataClusterAlloc = (PQEDCLUSTERASYNCALLOC)RTMemAllocZ(sizeof(QEDCLUSTERASYNCALLOC));
                        if (RT_UNLIKELY(!pDataClusterAlloc))
                        {
                            rc = VERR_NO_MEMORY;
                            break;
                        }

                        /* Allocate new cluster for the data. */
                        uint64_t offData = qedClusterAllocate(pImage, 1);

                        pDataClusterAlloc->enmAllocState = QEDCLUSTERASYNCALLOCSTATE_USER_ALLOC;
                        pDataClusterAlloc->cbImageOld    = offData;
                        pDataClusterAlloc->offClusterNew = offData;
                        pDataClusterAlloc->idxL1         = idxL1;
                        pDataClusterAlloc->idxL2         = idxL2;
                        pDataClusterAlloc->cbToWrite     = cbToWrite;
                        pDataClusterAlloc->pL2Entry      = pL2Entry;

                        /* Write data. */
                        rc = vdIfIoIntFileWriteUser(pImage->pIfIo, pImage->pStorage,
                                                    offData, pIoCtx, cbToWrite,
                                                    qedAsyncClusterAllocUpdate, pDataClusterAlloc);
                        if (rc == VERR_VD_ASYNC_IO_IN_PROGRESS)
                            break;
                        else if (RT_FAILURE(rc))
                        {
                            RTMemFree(pDataClusterAlloc);
                            break;
                        }

                        rc = qedAsyncClusterAllocUpdate(pImage, pIoCtx, pDataClusterAlloc, rc);
                    }
                }

            } while (0);

            *pcbPreRead = 0;
            *pcbPostRead = 0;
        }
        else
        {
            /* Trying to do a partial write to an unallocated cluster. Don't do
             * anything except letting the upper layer know what to do. */
            *pcbPreRead = offCluster;
            *pcbPostRead = pImage->cbCluster - cbToWrite - *pcbPreRead;
        }
    }

    if (pcbWriteProcess)
        *pcbWriteProcess = cbToWrite;


out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

static int qedFlush(void *pBackendData, PVDIOCTX pIoCtx)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    Assert(pImage);

    if (VALID_PTR(pIoCtx))
        rc = qedFlushImageAsync(pImage, pIoCtx);
    else
        rc = VERR_INVALID_PARAMETER;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetVersion */
static unsigned qedGetVersion(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;

    AssertPtr(pImage);

    if (pImage)
        return 1;
    else
        return 0;
}

/** @copydoc VBOXHDDBACKEND::pfnGetSectorSize */
static uint32_t qedGetSectorSize(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    uint32_t cb = 0;

    AssertPtr(pImage);

    if (pImage && pImage->pStorage)
        cb = 512;

    LogFlowFunc(("returns %u\n", cb));
    return cb;
}

/** @copydoc VBOXHDDBACKEND::pfnGetSize */
static uint64_t qedGetSize(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    uint64_t cb = 0;

    AssertPtr(pImage);

    if (pImage && pImage->pStorage)
        cb = pImage->cbSize;

    LogFlowFunc(("returns %llu\n", cb));
    return cb;
}

/** @copydoc VBOXHDDBACKEND::pfnGetFileSize */
static uint64_t qedGetFileSize(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    uint64_t cb = 0;

    AssertPtr(pImage);

    if (pImage)
    {
        uint64_t cbFile;
        if (pImage->pStorage)
        {
            int rc = vdIfIoIntFileGetSize(pImage->pIfIo, pImage->pStorage, &cbFile);
            if (RT_SUCCESS(rc))
                cb += cbFile;
        }
    }

    LogFlowFunc(("returns %lld\n", cb));
    return cb;
}

/** @copydoc VBOXHDDBACKEND::pfnGetPCHSGeometry */
static int qedGetPCHSGeometry(void *pBackendData,
                              PVDGEOMETRY pPCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pPCHSGeometry=%#p\n", pBackendData, pPCHSGeometry));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (pImage->PCHSGeometry.cCylinders)
        {
            *pPCHSGeometry = pImage->PCHSGeometry;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_VD_GEOMETRY_NOT_SET;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (PCHS=%u/%u/%u)\n", rc, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetPCHSGeometry */
static int qedSetPCHSGeometry(void *pBackendData,
                              PCVDGEOMETRY pPCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pPCHSGeometry=%#p PCHS=%u/%u/%u\n", pBackendData, pPCHSGeometry, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
        {
            rc = VERR_VD_IMAGE_READ_ONLY;
            goto out;
        }

        pImage->PCHSGeometry = *pPCHSGeometry;
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_VD_NOT_OPENED;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetLCHSGeometry */
static int qedGetLCHSGeometry(void *pBackendData,
                              PVDGEOMETRY pLCHSGeometry)
{
     LogFlowFunc(("pBackendData=%#p pLCHSGeometry=%#p\n", pBackendData, pLCHSGeometry));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (pImage->LCHSGeometry.cCylinders)
        {
            *pLCHSGeometry = pImage->LCHSGeometry;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_VD_GEOMETRY_NOT_SET;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (LCHS=%u/%u/%u)\n", rc, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetLCHSGeometry */
static int qedSetLCHSGeometry(void *pBackendData,
                               PCVDGEOMETRY pLCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pLCHSGeometry=%#p LCHS=%u/%u/%u\n", pBackendData, pLCHSGeometry, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
        {
            rc = VERR_VD_IMAGE_READ_ONLY;
            goto out;
        }

        pImage->LCHSGeometry = *pLCHSGeometry;
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_VD_NOT_OPENED;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetImageFlags */
static unsigned qedGetImageFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    unsigned uImageFlags;

    AssertPtr(pImage);

    if (pImage)
        uImageFlags = pImage->uImageFlags;
    else
        uImageFlags = 0;

    LogFlowFunc(("returns %#x\n", uImageFlags));
    return uImageFlags;
}

/** @copydoc VBOXHDDBACKEND::pfnGetOpenFlags */
static unsigned qedGetOpenFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    unsigned uOpenFlags;

    AssertPtr(pImage);

    if (pImage)
        uOpenFlags = pImage->uOpenFlags;
    else
        uOpenFlags = 0;

    LogFlowFunc(("returns %#x\n", uOpenFlags));
    return uOpenFlags;
}

/** @copydoc VBOXHDDBACKEND::pfnSetOpenFlags */
static int qedSetOpenFlags(void *pBackendData, unsigned uOpenFlags)
{
    LogFlowFunc(("pBackendData=%#p\n uOpenFlags=%#x", pBackendData, uOpenFlags));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    /* Image must be opened and the new flags must be valid. */
    if (!pImage || (uOpenFlags & ~(  VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_INFO
                                   | VD_OPEN_FLAGS_ASYNC_IO | VD_OPEN_FLAGS_SKIP_CONSISTENCY_CHECKS)))
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Implement this operation via reopening the image. */
    rc = qedFreeImage(pImage, false);
    if (RT_FAILURE(rc))
        goto out;
    rc = qedOpenImage(pImage, uOpenFlags);

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetComment */
static int qedGetComment(void *pBackendData, char *pszComment,
                          size_t cbComment)
{
    LogFlowFunc(("pBackendData=%#p pszComment=%#p cbComment=%zu\n", pBackendData, pszComment, cbComment));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc comment='%s'\n", rc, pszComment));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetComment */
static int qedSetComment(void *pBackendData, const char *pszComment)
{
    LogFlowFunc(("pBackendData=%#p pszComment=\"%s\"\n", pBackendData, pszComment));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
            rc = VERR_VD_IMAGE_READ_ONLY;
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetUuid */
static int qedGetUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetUuid */
static int qedSetUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    LogFlowFunc(("%RTuuid\n", pUuid));
    AssertPtr(pImage);

    if (pImage)
    {
        if (!(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetModificationUuid */
static int qedGetModificationUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetModificationUuid */
static int qedSetModificationUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (!(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetParentUuid */
static int qedGetParentUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetParentUuid */
static int qedSetParentUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (!(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetParentModificationUuid */
static int qedGetParentModificationUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetParentModificationUuid */
static int qedSetParentModificationUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (!(pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnDump */
static void qedDump(void *pBackendData)
{
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;

    AssertPtr(pImage);
    if (pImage)
    {
        vdIfErrorMessage(pImage->pIfError, "Header: Geometry PCHS=%u/%u/%u LCHS=%u/%u/%u cbSector=%llu\n",
                         pImage->PCHSGeometry.cCylinders, pImage->PCHSGeometry.cHeads, pImage->PCHSGeometry.cSectors,
                         pImage->LCHSGeometry.cCylinders, pImage->LCHSGeometry.cHeads, pImage->LCHSGeometry.cSectors,
                         pImage->cbSize / 512);
    }
}

/** @copydoc VBOXHDDBACKEND::pfnGetParentFilename */
static int qedGetParentFilename(void *pBackendData, char **ppszParentFilename)
{
    int rc = VINF_SUCCESS;
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;

    AssertPtr(pImage);
    if (pImage)
        if (pImage->pszBackingFilename)
            *ppszParentFilename = RTStrDup(pImage->pszBackingFilename);
        else
            rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetParentFilename */
static int qedSetParentFilename(void *pBackendData, const char *pszParentFilename)
{
    int rc = VINF_SUCCESS;
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;

    AssertPtr(pImage);
    if (pImage)
    {
        if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
            rc = VERR_VD_IMAGE_READ_ONLY;
        else if (   pImage->pszBackingFilename
                 && (strlen(pszParentFilename) > pImage->cbBackingFilename))
            rc = VERR_NOT_SUPPORTED; /* The new filename is longer than the old one. */
        else
        {
            if (pImage->pszBackingFilename)
                RTStrFree(pImage->pszBackingFilename);
            pImage->pszBackingFilename = RTStrDup(pszParentFilename);
            if (!pImage->pszBackingFilename)
                rc = VERR_NO_MEMORY;
            else
            {
                if (!pImage->offBackingFilename)
                {
                    /* Allocate new cluster. */
                    uint64_t offData = qedClusterAllocate(pImage, 1);

                    Assert((offData & UINT32_MAX) == offData);
                    pImage->offBackingFilename = (uint32_t)offData;
                    pImage->cbBackingFilename  = (uint32_t)strlen(pszParentFilename);
                    rc = vdIfIoIntFileSetSize(pImage->pIfIo, pImage->pStorage,
                                              offData + pImage->cbCluster);
                }

                if (RT_SUCCESS(rc))
                    rc = vdIfIoIntFileWriteSync(pImage->pIfIo, pImage->pStorage,
                                                pImage->offBackingFilename,
                                                pImage->pszBackingFilename,
                                                strlen(pImage->pszBackingFilename));
            }
        }
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnResize */
static int qedResize(void *pBackendData, uint64_t cbSize,
                     PCVDGEOMETRY pPCHSGeometry, PCVDGEOMETRY pLCHSGeometry,
                     unsigned uPercentStart, unsigned uPercentSpan,
                     PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                     PVDINTERFACE pVDIfsOperation)
{
    PQEDIMAGE pImage = (PQEDIMAGE)pBackendData;
    int rc = VINF_SUCCESS;

    PVDINTERFACEPROGRESS pIfProgress = VDIfProgressGet(pVDIfsOperation);

    /* Making the image smaller is not supported at the moment. */
    if (cbSize < pImage->cbSize)
        rc = VERR_NOT_SUPPORTED;
    else if (cbSize > pImage->cbSize)
    {
        /*
         * It is enough to just update the size field in the header to complete
         * growing. With the default cluster and table sizes the image can be expanded
         * to 64TB without overflowing the L1 and L2 tables making block relocation
         * superfluous.
         * @todo: The rare case where block relocation is still required (non default
         *        table and/or cluster size or images with more than 64TB) is not
         *        implemented yet and resizing such an image will fail with an error.
         */
        if (qedByte2Cluster(pImage, pImage->cbTable)*pImage->cTableEntries*pImage->cTableEntries*pImage->cbCluster < cbSize)
            rc = vdIfError(pImage->pIfError, VERR_BUFFER_OVERFLOW, RT_SRC_POS,
                           N_("Qed: Resizing the image '%s' is not supported because it would overflow the L1 and L2 table\n"),
                           pImage->pszFilename);
        else
        {
            uint64_t cbSizeOld = pImage->cbSize;

            pImage->cbSize = cbSize;
            rc = qedFlushImage(pImage);
            if (RT_FAILURE(rc))
            {
                pImage->cbSize = cbSizeOld; /* Restore */

                rc = vdIfError(pImage->pIfError, rc, RT_SRC_POS, N_("Qed: Resizing the image '%s' failed\n"),
                               pImage->pszFilename);
            }
        }
    }
    /* Same size doesn't change the image at all. */

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}


const VBOXHDDBACKEND g_QedBackend =
{
    /* pszBackendName */
    "QED",
    /* cbSize */
    sizeof(VBOXHDDBACKEND),
    /* uBackendCaps */
    VD_CAP_FILE | VD_CAP_VFS | VD_CAP_CREATE_DYNAMIC | VD_CAP_DIFF | VD_CAP_ASYNC,
    /* paFileExtensions */
    s_aQedFileExtensions,
    /* paConfigInfo */
    NULL,
    /* pfnCheckIfValid */
    qedCheckIfValid,
    /* pfnOpen */
    qedOpen,
    /* pfnCreate */
    qedCreate,
    /* pfnRename */
    qedRename,
    /* pfnClose */
    qedClose,
    /* pfnRead */
    qedRead,
    /* pfnWrite */
    qedWrite,
    /* pfnFlush */
    qedFlush,
    /* pfnDiscard */
    NULL,
    /* pfnGetVersion */
    qedGetVersion,
    /* pfnGetSectorSize */
    qedGetSectorSize,
    /* pfnGetSize */
    qedGetSize,
    /* pfnGetFileSize */
    qedGetFileSize,
    /* pfnGetPCHSGeometry */
    qedGetPCHSGeometry,
    /* pfnSetPCHSGeometry */
    qedSetPCHSGeometry,
    /* pfnGetLCHSGeometry */
    qedGetLCHSGeometry,
    /* pfnSetLCHSGeometry */
    qedSetLCHSGeometry,
    /* pfnGetImageFlags */
    qedGetImageFlags,
    /* pfnGetOpenFlags */
    qedGetOpenFlags,
    /* pfnSetOpenFlags */
    qedSetOpenFlags,
    /* pfnGetComment */
    qedGetComment,
    /* pfnSetComment */
    qedSetComment,
    /* pfnGetUuid */
    qedGetUuid,
    /* pfnSetUuid */
    qedSetUuid,
    /* pfnGetModificationUuid */
    qedGetModificationUuid,
    /* pfnSetModificationUuid */
    qedSetModificationUuid,
    /* pfnGetParentUuid */
    qedGetParentUuid,
    /* pfnSetParentUuid */
    qedSetParentUuid,
    /* pfnGetParentModificationUuid */
    qedGetParentModificationUuid,
    /* pfnSetParentModificationUuid */
    qedSetParentModificationUuid,
    /* pfnDump */
    qedDump,
    /* pfnGetTimeStamp */
    NULL,
    /* pfnGetParentTimeStamp */
    NULL,
    /* pfnSetParentTimeStamp */
    NULL,
    /* pfnGetParentFilename */
    qedGetParentFilename,
    /* pfnSetParentFilename */
    qedSetParentFilename,
    /* pfnComposeLocation */
    genericFileComposeLocation,
    /* pfnComposeName */
    genericFileComposeName,
    /* pfnCompact */
    NULL,
    /* pfnResize */
    qedResize,
    /* pfnRepair */
    NULL,
    /* pfnTraverseMetadata */
    NULL
};
