/*
 * Copyright © 2017 Advanced Micro Devices, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */

/**
************************************************************************************************************************
* @file  addrlib2.h
* @brief Contains the Addr::V2::Lib class definition.
************************************************************************************************************************
*/

#ifndef __ADDR2_LIB2_H__
#define __ADDR2_LIB2_H__

#include "addrlib.h"

namespace Addr
{
namespace V2
{

/**
************************************************************************************************************************
* @brief Flags for SwizzleModeTable
************************************************************************************************************************
*/
struct SwizzleModeFlags
{
    // Swizzle mode
    UINT_32 isLinear        : 1;    // Linear

    // Block size
    UINT_32 is256b          : 1;    // Block size is 256B
    UINT_32 is4kb           : 1;    // Block size is 4KB
    UINT_32 is64kb          : 1;    // Block size is 64KB
    UINT_32 isVar           : 1;    // Block size is variable

    UINT_32 isZ             : 1;    // Z order swizzle mode
    UINT_32 isStd           : 1;    // Standard swizzle mode
    UINT_32 isDisp          : 1;    // Display swizzle mode
    UINT_32 isRot           : 1;    // Rotate swizzle mode

    // XOR mode
    UINT_32 isXor           : 1;    // XOR after swizzle if set

    UINT_32 isT             : 1;    // T mode

    UINT_32 isRtOpt         : 1;    // mode opt for render target
};

struct Dim2d
{
    UINT_32 w;
    UINT_32 h;
};

struct Dim3d
{
    UINT_32 w;
    UINT_32 h;
    UINT_32 d;
};

/**
************************************************************************************************************************
* @brief This class contains asic independent address lib functionalities
************************************************************************************************************************
*/
class Lib : public Addr::Lib
{
public:
    virtual ~Lib();

    static Lib* GetLib(
        ADDR_HANDLE hLib);

    //
    // Interface stubs
    //

    // For data surface
    ADDR_E_RETURNCODE ComputeSurfaceInfo(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceAddrFromCoord(
        const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceCoordFromAddr(
        const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT*  pIn,
        ADDR2_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT* pOut) const;

    // For HTile
    ADDR_E_RETURNCODE ComputeHtileInfo(
        const ADDR2_COMPUTE_HTILE_INFO_INPUT* pIn,
        ADDR2_COMPUTE_HTILE_INFO_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeHtileAddrFromCoord(
        const ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeHtileCoordFromAddr(
        const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT*  pIn,
        ADDR2_COMPUTE_HTILE_COORDFROMADDR_OUTPUT* pOut) const;

    // For CMask
    ADDR_E_RETURNCODE ComputeCmaskInfo(
        const ADDR2_COMPUTE_CMASK_INFO_INPUT* pIn,
        ADDR2_COMPUTE_CMASK_INFO_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeCmaskAddrFromCoord(
        const ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeCmaskCoordFromAddr(
        const ADDR2_COMPUTE_CMASK_COORDFROMADDR_INPUT*  pIn,
        ADDR2_COMPUTE_CMASK_COORDFROMADDR_OUTPUT* pOut) const;

    // For FMask
    ADDR_E_RETURNCODE ComputeFmaskInfo(
        const ADDR2_COMPUTE_FMASK_INFO_INPUT*  pIn,
        ADDR2_COMPUTE_FMASK_INFO_OUTPUT* pOut);

    ADDR_E_RETURNCODE ComputeFmaskAddrFromCoord(
        const ADDR2_COMPUTE_FMASK_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_FMASK_ADDRFROMCOORD_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeFmaskCoordFromAddr(
        const ADDR2_COMPUTE_FMASK_COORDFROMADDR_INPUT*  pIn,
        ADDR2_COMPUTE_FMASK_COORDFROMADDR_OUTPUT* pOut) const;

    // For DCC key
    ADDR_E_RETURNCODE ComputeDccInfo(
        const ADDR2_COMPUTE_DCCINFO_INPUT* pIn,
        ADDR2_COMPUTE_DCCINFO_OUTPUT* pOut) const;

    ADDR_E_RETURNCODE ComputeDccAddrFromCoord(
        const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_DCC_ADDRFROMCOORD_OUTPUT* pOut) const;

    // Misc
    ADDR_E_RETURNCODE ComputePipeBankXor(
        const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,
        ADDR2_COMPUTE_PIPEBANKXOR_OUTPUT*      pOut);

    ADDR_E_RETURNCODE ComputeSlicePipeBankXor(
        const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,
        ADDR2_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT*      pOut);

    ADDR_E_RETURNCODE ComputeSubResourceOffsetForSwizzlePattern(
        const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,
        ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT*      pOut);

    ADDR_E_RETURNCODE Addr2GetPreferredSurfaceSetting(
        const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,
        ADDR2_GET_PREFERRED_SURF_SETTING_OUTPUT*      pOut) const;

    virtual BOOL_32 IsValidDisplaySwizzleMode(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTIMPLEMENTED;
    }

protected:
    Lib();  // Constructor is protected
    Lib(const Client* pClient);

    static const UINT_32 MaxNumOfBpp = 5;

    static const Dim2d Block256_2d[MaxNumOfBpp];
    static const Dim3d Block1K_3d[MaxNumOfBpp];

    static const UINT_32 PrtAlignment = 64 * 1024;
    static const UINT_32 MaxMacroBits = 20;

    // Checking block size
    BOOL_32 IsBlock256b(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].is256b;
    }

    BOOL_32 IsBlock4kb(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].is4kb;
    }

    BOOL_32 IsBlock64kb(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].is64kb;
    }

    BOOL_32 IsBlockVariable(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isVar;
    }

    // Checking swizzle mode
    BOOL_32 IsLinear(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isLinear;
    }

    BOOL_32 IsRtOptSwizzle(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isRtOpt;
    }

    BOOL_32 IsZOrderSwizzle(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isZ;
    }

    BOOL_32 IsStandardSwizzle(AddrResourceType resourceType, AddrSwizzleMode swizzleMode) const
    {
        return HwlIsStandardSwizzle(resourceType, swizzleMode);
    }

    BOOL_32 IsDisplaySwizzle(AddrResourceType resourceType, AddrSwizzleMode swizzleMode) const
    {
        return HwlIsDisplaySwizzle(resourceType, swizzleMode);
    }

    BOOL_32 IsRotateSwizzle(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isRot;
    }

    BOOL_32 IsXor(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isXor;
    }

    BOOL_32 IsPrt(AddrSwizzleMode swizzleMode) const
    {
        return m_swizzleModeTable[swizzleMode].isT;
    }

    BOOL_32 IsNonPrtXor(AddrSwizzleMode swizzleMode) const
    {
        return (IsXor(swizzleMode) && (IsPrt(swizzleMode) == FALSE));
    }

    // Checking resource type
    static BOOL_32 IsTex1d(AddrResourceType resourceType)
    {
        return (resourceType == ADDR_RSRC_TEX_1D);
    }

    static BOOL_32 IsTex2d(AddrResourceType resourceType)
    {
        return (resourceType == ADDR_RSRC_TEX_2D);
    }

    static BOOL_32 IsTex3d(AddrResourceType resourceType)
    {
        return (resourceType == ADDR_RSRC_TEX_3D);
    }

    BOOL_32 IsThick(AddrResourceType resourceType, AddrSwizzleMode swizzleMode) const
    {
        return HwlIsThick(resourceType, swizzleMode);
    }

    BOOL_32 IsThin(AddrResourceType resourceType, AddrSwizzleMode swizzleMode) const
    {
        return HwlIsThin(resourceType, swizzleMode);
    }

    UINT_32 GetBlockSizeLog2(AddrSwizzleMode swizzleMode) const
    {
        UINT_32 blockSizeLog2 = 0;

        if (IsBlock256b(swizzleMode) || IsLinear(swizzleMode))
        {
            blockSizeLog2 = 8;
        }
        else if (IsBlock4kb(swizzleMode))
        {
            blockSizeLog2 = 12;
        }
        else if (IsBlock64kb(swizzleMode))
        {
            blockSizeLog2 = 16;
        }
        else if (IsBlockVariable(swizzleMode))
        {
            blockSizeLog2 = m_blockVarSizeLog2;
        }
        else
        {
            ADDR_ASSERT_ALWAYS();
        }

        return blockSizeLog2;
    }

    UINT_32 GetBlockSize(AddrSwizzleMode swizzleMode) const
    {
        return (1 << GetBlockSizeLog2(swizzleMode));
    }

    static UINT_32 GetFmaskBpp(UINT_32 sample, UINT_32 frag)
    {
        sample = (sample == 0) ? 1 : sample;
        frag   = (frag   == 0) ? sample : frag;

        UINT_32 fmaskBpp = QLog2(frag);

        if (sample > frag)
        {
            fmaskBpp++;
        }

        if (fmaskBpp == 3)
        {
            fmaskBpp = 4;
        }

        fmaskBpp = Max(8u, fmaskBpp * sample);

        return fmaskBpp;
    }

    virtual BOOL_32 HwlIsStandardSwizzle(
        AddrResourceType resourceType,
        AddrSwizzleMode  swizzleMode) const
    {
        ADDR_NOT_IMPLEMENTED();
        return FALSE;
    }

    virtual BOOL_32 HwlIsDisplaySwizzle(
        AddrResourceType resourceType,
        AddrSwizzleMode  swizzleMode) const
    {
        ADDR_NOT_IMPLEMENTED();
        return FALSE;
    }

    virtual BOOL_32 HwlIsThin(
        AddrResourceType resourceType,
        AddrSwizzleMode  swizzleMode) const
    {
        ADDR_NOT_IMPLEMENTED();
        return FALSE;
    }

    virtual BOOL_32 HwlIsThick(
        AddrResourceType resourceType,
        AddrSwizzleMode  swizzleMode) const
    {
        ADDR_NOT_IMPLEMENTED();
        return FALSE;
    }

    virtual ADDR_E_RETURNCODE HwlComputeHtileInfo(
        const ADDR2_COMPUTE_HTILE_INFO_INPUT*    pIn,
        ADDR2_COMPUTE_HTILE_INFO_OUTPUT*         pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeCmaskInfo(
        const ADDR2_COMPUTE_CMASK_INFO_INPUT*    pIn,
        ADDR2_COMPUTE_CMASK_INFO_OUTPUT*         pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeDccInfo(
        const ADDR2_COMPUTE_DCCINFO_INPUT*    pIn,
        ADDR2_COMPUTE_DCCINFO_OUTPUT*         pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeDccAddrFromCoord(
        const ADDR2_COMPUTE_DCC_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_DCC_ADDRFROMCOORD_OUTPUT* pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeCmaskAddrFromCoord(
        const ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_CMASK_ADDRFROMCOORD_OUTPUT* pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeHtileAddrFromCoord(
        const ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_INPUT*  pIn,
        ADDR2_COMPUTE_HTILE_ADDRFROMCOORD_OUTPUT* pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeHtileCoordFromAddr(
        const ADDR2_COMPUTE_HTILE_COORDFROMADDR_INPUT*  pIn,
        ADDR2_COMPUTE_HTILE_COORDFROMADDR_OUTPUT* pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeBlock256Equation(
        AddrResourceType rsrcType,
        AddrSwizzleMode swMode,
        UINT_32 elementBytesLog2,
        ADDR_EQUATION* pEquation) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeThinEquation(
        AddrResourceType rsrcType,
        AddrSwizzleMode swMode,
        UINT_32 elementBytesLog2,
        ADDR_EQUATION* pEquation) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeThickEquation(
        AddrResourceType rsrcType,
        AddrSwizzleMode swMode,
        UINT_32 elementBytesLog2,
        ADDR_EQUATION* pEquation) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual UINT_32 HwlGetEquationIndex(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_INVALID_EQUATION_INDEX;
    }

    UINT_32 GetEquationIndex(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut) const
    {
        return HwlGetEquationIndex(pIn, pOut);
    }

    virtual UINT_32 HwlComputeSurfaceBaseAlign(AddrSwizzleMode swizzleMode) const
    {
        ADDR_NOT_IMPLEMENTED();
        return 0;
    }

    virtual ADDR_E_RETURNCODE HwlComputePipeBankXor(
        const ADDR2_COMPUTE_PIPEBANKXOR_INPUT* pIn,
        ADDR2_COMPUTE_PIPEBANKXOR_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeSlicePipeBankXor(
        const ADDR2_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,
        ADDR2_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }


    virtual ADDR_E_RETURNCODE HwlComputeSubResourceOffsetForSwizzlePattern(
        const ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,
        ADDR2_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlGetPreferredSurfaceSetting(
        const ADDR2_GET_PREFERRED_SURF_SETTING_INPUT* pIn,
        ADDR2_GET_PREFERRED_SURF_SETTING_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeSurfaceInfoSanityCheck(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTSUPPORTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeSurfaceInfoTiled(
         const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
         ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTIMPLEMENTED;
    }

    virtual ADDR_E_RETURNCODE HwlComputeSurfaceAddrFromCoordTiled(
        const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut) const
    {
        ADDR_NOT_IMPLEMENTED();
        return ADDR_NOTIMPLEMENTED;
    }

    ADDR_E_RETURNCODE ComputeBlock256Equation(
        AddrResourceType rsrcType,
        AddrSwizzleMode swMode,
        UINT_32 elementBytesLog2,
        ADDR_EQUATION* pEquation) const;

    ADDR_E_RETURNCODE ComputeThinEquation(
        AddrResourceType rsrcType,
        AddrSwizzleMode swMode,
        UINT_32 elementBytesLog2,
        ADDR_EQUATION* pEquation) const;

    ADDR_E_RETURNCODE ComputeThickEquation(
        AddrResourceType rsrcType,
        AddrSwizzleMode swMode,
        UINT_32 elementBytesLog2,
        ADDR_EQUATION* pEquation) const;

    ADDR_E_RETURNCODE ComputeSurfaceInfoSanityCheck(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn) const;

    ADDR_E_RETURNCODE ComputeSurfaceInfoLinear(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceLinearPadding(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        UINT_32* pMipmap0PaddedWidth,
        UINT_32* pSlice0PaddedHeight,
        ADDR2_MIP_INFO* pMipInfo = NULL) const;

    ADDR_E_RETURNCODE ComputeSurfaceInfoTiled(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_INFO_OUTPUT*      pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceAddrFromCoordLinear(
        const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceAddrFromCoordTiled(
        const ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceCoordFromAddrLinear(
        const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut) const;

    ADDR_E_RETURNCODE ComputeSurfaceCoordFromAddrTiled(
        const ADDR2_COMPUTE_SURFACE_COORDFROMADDR_INPUT* pIn,
        ADDR2_COMPUTE_SURFACE_COORDFROMADDR_OUTPUT*      pOut) const;

    UINT_32 ComputeSurface2DMicroBlockOffset(
        const _ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn) const;

    UINT_32 ComputeSurface3DMicroBlockOffset(
        const _ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn) const;

    // Misc
    ADDR_E_RETURNCODE ComputeBlockDimensionForSurf(
        UINT_32*         pWidth,
        UINT_32*         pHeight,
        UINT_32*         pDepth,
        UINT_32          bpp,
        UINT_32          numSamples,
        AddrResourceType resourceType,
        AddrSwizzleMode  swizzleMode) const;

    ADDR_E_RETURNCODE ComputeBlockDimension(
        UINT_32*          pWidth,
        UINT_32*          pHeight,
        UINT_32*          pDepth,
        UINT_32           bpp,
        AddrResourceType  resourceType,
        AddrSwizzleMode   swizzleMode) const;

    static UINT_64 ComputePadSize(
        const Dim3d*      pBlkDim,
        UINT_32           width,
        UINT_32           height,
        UINT_32           numSlices,
        Dim3d*            pPadDim)
    {
        pPadDim->w = PowTwoAlign(width ,pBlkDim->w);
        pPadDim->h = PowTwoAlign(height ,pBlkDim->h);
        pPadDim->d = PowTwoAlign(numSlices, pBlkDim->d);
        return static_cast<UINT_64>(pPadDim->w) * pPadDim->h * pPadDim->d;
    }

    static ADDR_E_RETURNCODE ExtractPipeBankXor(
        UINT_32  pipeBankXor,
        UINT_32  bankBits,
        UINT_32  pipeBits,
        UINT_32* pBankX,
        UINT_32* pPipeX);

    static BOOL_32 Valid3DMipSliceIdConstraint(
        UINT_32 numSlices,
        UINT_32 mipId,
        UINT_32 slice)
    {
        return (Max((numSlices >> mipId), 1u) > slice);
    }

    Dim3d GetMipTailDim(
        AddrResourceType  resourceType,
        AddrSwizzleMode   swizzleMode,
        UINT_32           blockWidth,
        UINT_32           blockHeight,
        UINT_32           blockDepth) const;

    BOOL_32 IsInMipTail(
        AddrResourceType  resourceType,
        AddrSwizzleMode   swizzleMode,
        Dim3d             mipTailDim,
        UINT_32           width,
        UINT_32           height,
        UINT_32           depth) const
    {
        BOOL_32 inTail = ((width <= mipTailDim.w) &&
                          (height <= mipTailDim.h) &&
                          (IsThin(resourceType, swizzleMode) || (depth <= mipTailDim.d)));

        return inTail;
    }

    static BOOL_32 IsLocalHeap(AddrResrouceLocation resourceType)
    {
        return ((resourceType == ADDR_RSRC_LOC_LOCAL) ||
                (resourceType == ADDR_RSRC_LOC_INVIS));
    }

    static BOOL_32 IsInvisibleHeap(AddrResrouceLocation resourceType)
    {
        return (resourceType == ADDR_RSRC_LOC_INVIS);
    }

    static BOOL_32 IsNonlocalHeap(AddrResrouceLocation resourceType)
    {
        return ((resourceType == ADDR_RSRC_LOC_USWC) ||
                (resourceType == ADDR_RSRC_LOC_CACHED));
    }

    UINT_32 GetPipeLog2ForMetaAddressing(BOOL_32 pipeAligned, AddrSwizzleMode swizzleMode) const
    {
        UINT_32 numPipeLog2 = pipeAligned ? Min(m_pipesLog2 + m_seLog2, 5u) : 0;

        if (IsXor(swizzleMode))
        {
            UINT_32 maxPipeLog2 = GetBlockSizeLog2(swizzleMode) - m_pipeInterleaveLog2;

            numPipeLog2 = Min(numPipeLog2, maxPipeLog2);
        }

        return numPipeLog2;
    }

    UINT_32 GetPipeNumForMetaAddressing(BOOL_32 pipeAligned, AddrSwizzleMode swizzleMode) const
    {
        return (1 << GetPipeLog2ForMetaAddressing(pipeAligned, swizzleMode));
    }

    VOID VerifyMipLevelInfo(const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn) const
    {
#if DEBUG
        if (pIn->numMipLevels > 1)
        {
            UINT_32 actualMipLevels = 1;
            switch (pIn->resourceType)
            {
                case ADDR_RSRC_TEX_3D:
                    // Fall through to share 2D case
                    actualMipLevels = Max(actualMipLevels, Log2NonPow2(pIn->numSlices) + 1);
                case ADDR_RSRC_TEX_2D:
                    // Fall through to share 1D case
                    actualMipLevels = Max(actualMipLevels, Log2NonPow2(pIn->height) + 1);
                case ADDR_RSRC_TEX_1D:
                    // Base 1D case
                    actualMipLevels = Max(actualMipLevels, Log2NonPow2(pIn->width) + 1);
                    break;
                default:
                    ADDR_ASSERT_ALWAYS();
                    break;
            }
            // Client pass wrong number of MipLevels to addrlib and result will be bad.
            // Not sure if we should fail this calling instead of putting an assertion here.
            ADDR_ASSERT(actualMipLevels >= pIn->numMipLevels);
        }
#endif
    }

    ADDR_E_RETURNCODE ApplyCustomerPipeBankXor(
        AddrSwizzleMode swizzleMode,
        UINT_32         pipeBankXor,
        UINT_32         bankBits,
        UINT_32         pipeBits,
        UINT_32*        pBlockOffset) const
    {
        ADDR_E_RETURNCODE returnCode = ADDR_OK;

        if (IsXor(swizzleMode))
        {
            // Apply driver set bankPipeXor
            UINT_32 bankX = 0;
            UINT_32 pipeX = 0;
            returnCode = ExtractPipeBankXor(pipeBankXor, bankBits, pipeBits, &bankX, &pipeX);
            *pBlockOffset ^= (pipeX << m_pipeInterleaveLog2);
            *pBlockOffset ^= (bankX << (m_pipeInterleaveLog2 + pipeBits));
        }

        return returnCode;
    }

    UINT_32 GetPipeXorBits(UINT_32 macroBlockBits) const;
    UINT_32 GetBankXorBits(UINT_32 macroBlockBits) const;

    ADDR_E_RETURNCODE ApplyCustomizedPitchHeight(
        const ADDR2_COMPUTE_SURFACE_INFO_INPUT* pIn,
        UINT_32  elementBytes,
        UINT_32  pitchAlignInElement,
        UINT_32* pPitch,
        UINT_32* pHeight) const;

    VOID ComputeQbStereoInfo(ADDR2_COMPUTE_SURFACE_INFO_OUTPUT* pOut) const;

    UINT_32 m_se;                       ///< Number of shader engine
    UINT_32 m_rbPerSe;                  ///< Number of render backend per shader engine
    UINT_32 m_maxCompFrag;              ///< Number of max compressed fragment

    UINT_32 m_banksLog2;                ///< Number of bank Log2
    UINT_32 m_pipesLog2;                ///< Number of pipe per shader engine Log2
    UINT_32 m_seLog2;                   ///< Number of shader engine Log2
    UINT_32 m_rbPerSeLog2;              ///< Number of render backend per shader engine Log2
    UINT_32 m_maxCompFragLog2;          ///< Number of max compressed fragment Log2

    UINT_32 m_pipeInterleaveLog2;       ///< Log2 of pipe interleave bytes

    UINT_32 m_blockVarSizeLog2;         ///< Log2 of block var size

    SwizzleModeFlags m_swizzleModeTable[ADDR_SW_MAX_TYPE];  ///< Swizzle mode table

private:
    // Disallow the copy constructor
    Lib(const Lib& a);

    // Disallow the assignment operator
    Lib& operator=(const Lib& a);
};

} // V2
} // Addr

#endif

