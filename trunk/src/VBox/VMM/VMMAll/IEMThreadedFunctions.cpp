/* $Id$ */
/** @file
 * IEM - Instruction Decoding and Emulation, Threaded Functions.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifndef LOG_GROUP /* defined when included by tstIEMCheckMc.cpp */
# define LOG_GROUP LOG_GROUP_IEM
#endif
#define VMCPU_INCL_CPUM_GST_CTX
#define IEM_WITH_OPAQUE_DECODER_STATE
#include <VBox/vmm/iem.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/nem.h>
#include <VBox/vmm/gim.h>
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
# include <VBox/vmm/em.h>
# include <VBox/vmm/hm_svm.h>
#endif
#ifdef VBOX_WITH_NESTED_HWVIRT_VMX
# include <VBox/vmm/hmvmxinline.h>
#endif
#include <VBox/vmm/tm.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/dbgftrace.h>
#include "IEMInternal.h"
#include <VBox/vmm/vmcc.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/dis.h>
#include <VBox/disopcode-x86-amd64.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/x86.h>

#include "IEMInline.h"
#include "IEMMc.h"

#include "IEMThreadedFunctions.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Variant of IEM_MC_ADVANCE_RIP_AND_FINISH with instruction length as param. */
#define IEM_MC_ADVANCE_RIP_AND_FINISH_THREADED(a_cbInstr) \
    return iemRegAddToRipAndFinishingClearingRF(pVCpu, a_cbInstr)
#undef IEM_MC_ADVANCE_RIP_AND_FINISH

/** Variant of IEM_MC_REL_JMP_S8_AND_FINISH with instruction length as param. */
#define IEM_MC_REL_JMP_S8_AND_FINISH_THREADED(a_i8, a_cbInstr) \
    return iemRegRipRelativeJumpS8AndFinishClearingRF(pVCpu, a_cbInstr, (a_i8), pVCpu->iem.s.enmEffOpSize)
#undef IEM_MC_REL_JMP_S8_AND_FINISH

/** Variant of IEM_MC_REL_JMP_S16_AND_FINISH with instruction length as param. */
#define IEM_MC_REL_JMP_S16_AND_FINISH_THREADED(a_i16, a_cbInstr) \
    return iemRegRipRelativeJumpS16AndFinishClearingRF(pVCpu, a_cbInstr, (a_i16))
#undef IEM_MC_REL_JMP_S16_AND_FINISH

/** Variant of IEM_MC_REL_JMP_S32_AND_FINISH with instruction length as param. */
#define IEM_MC_REL_JMP_S32_AND_FINISH_THREADED(a_i32, a_cbInstr) \
    return iemRegRipRelativeJumpS32AndFinishClearingRF(pVCpu, a_cbInstr, (a_i32), pVCpu->iem.s.enmEffOpSize)
#undef IEM_MC_REL_JMP_S32_AND_FINISH

/** Variant of IEM_MC_CALC_RM_EFF_ADDR with additional parameters. */
# define IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR16(a_GCPtrEff, a_bRm, a_u16Disp) \
    (a_GCPtrEff) = iemOpHlpCalcRmEffAddrThreadedAddr16(pVCpu, a_bRm, a_u16Disp)

/** Variant of IEM_MC_CALC_RM_EFF_ADDR with additional parameters. */
# define IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR32(a_GCPtrEff, a_bRm, a_bSib, a_u32Disp) \
    (a_GCPtrEff) = iemOpHlpCalcRmEffAddrThreadedAddr32(pVCpu, a_bRm, a_bSib, a_u32Disp)

/** Variant of IEM_MC_CALC_RM_EFF_ADDR with additional parameters. */
# define IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR32FLAT(a_GCPtrEff, a_bRm, a_bSib, a_u32Disp) \
    (a_GCPtrEff) = iemOpHlpCalcRmEffAddrThreadedAddr32(pVCpu, a_bRm, a_bSib, a_u32Disp)

/** Variant of IEM_MC_CALC_RM_EFF_ADDR with additional parameters. */
# define IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR64(a_GCPtrEff, a_bRmEx, a_bSib, a_u32Disp, a_cbImm) \
    (a_GCPtrEff) = iemOpHlpCalcRmEffAddrThreadedAddr64(pVCpu, a_bRmEx, a_bSib, a_u32Disp, a_cbImm)

/** Variant of IEM_MC_CALC_RM_EFF_ADDR with additional parameters. */
# define IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR6432(a_GCPtrEff, a_bRmEx, a_bSib, a_u32Disp, a_cbImm) \
    (a_GCPtrEff) = (uint32_t)iemOpHlpCalcRmEffAddrThreadedAddr64(pVCpu, a_bRmEx, a_bSib, a_u32Disp, a_cbImm)

/**
 * Calculates the effective address of a ModR/M memory operand, 16-bit
 * addressing variant.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR16.
 *
 * @returns The effective address.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   u16Disp             The displacement byte/word, if any.
 *                              RIP relative addressing.
 */
static RTGCPTR iemOpHlpCalcRmEffAddrThreadedAddr16(PVMCPUCC pVCpu, uint8_t bRm, uint16_t u16Disp) RT_NOEXCEPT
{
    Log5(("iemOpHlpCalcRmEffAddrThreadedAddr16: bRm=%#x\n", bRm));
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

    /* Handle the disp16 form with no registers first. */
    if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 6)
    {
        Log5(("iemOpHlpCalcRmEffAddrThreadedAddr16: EffAddr=%#010RGv\n", (RTGCPTR)u16Disp));
        return u16Disp;
    }

    /* Get the displacment. */
    /** @todo we can eliminate this step by making u16Disp have this value
     *        already! */
    uint16_t u16EffAddr;
    switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
    {
        case 0:  u16EffAddr = 0;                        break;
        case 1:  u16EffAddr = (int16_t)(int8_t)u16Disp; break;
        case 2:  u16EffAddr = u16Disp;                  break;
        default: AssertFailedStmt(u16EffAddr = 0);
    }

    /* Add the base and index registers to the disp. */
    switch (bRm & X86_MODRM_RM_MASK)
    {
        case 0: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.si; break;
        case 1: u16EffAddr += pVCpu->cpum.GstCtx.bx + pVCpu->cpum.GstCtx.di; break;
        case 2: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.si; break;
        case 3: u16EffAddr += pVCpu->cpum.GstCtx.bp + pVCpu->cpum.GstCtx.di; break;
        case 4: u16EffAddr += pVCpu->cpum.GstCtx.si; break;
        case 5: u16EffAddr += pVCpu->cpum.GstCtx.di; break;
        case 6: u16EffAddr += pVCpu->cpum.GstCtx.bp; break;
        case 7: u16EffAddr += pVCpu->cpum.GstCtx.bx; break;
    }

    Log5(("iemOpHlpCalcRmEffAddrThreadedAddr16: EffAddr=%#010RGv\n", (RTGCPTR)u16EffAddr));
    return u16EffAddr;
}


/**
 * Calculates the effective address of a ModR/M memory operand, 32-bit
 * addressing variant.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR32 and
 * IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR32FLAT.
 *
 * @returns The effective address.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRm                 The ModRM byte.
 * @param   bSib                The SIB byte, if any.
 * @param   u32Disp             The displacement byte/dword, if any.
 */
static RTGCPTR iemOpHlpCalcRmEffAddrThreadedAddr32(PVMCPUCC pVCpu, uint8_t bRm, uint8_t bSib, uint32_t u32Disp) RT_NOEXCEPT
{
    Log5(("iemOpHlpCalcRmEffAddrThreadedAddr32: bRm=%#x\n", bRm));
    Assert(pVCpu->iem.s.enmCpuMode != IEMMODE_64BIT);

    /* Handle the disp32 form with no registers first. */
    if ((bRm & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
    {
        Log5(("iemOpHlpCalcRmEffAddrThreadedAddr32: EffAddr=%#010RGv\n", (RTGCPTR)u32Disp));
        return u32Disp;
    }

    /* Get the register (or SIB) value. */
    uint32_t u32EffAddr;
    switch (bRm & X86_MODRM_RM_MASK)
    {
        case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
        case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
        case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
        case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
        case 4: /* SIB */
        {
            /* Get the index and scale it. */
            switch ((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK)
            {
                case 0: u32EffAddr = pVCpu->cpum.GstCtx.eax; break;
                case 1: u32EffAddr = pVCpu->cpum.GstCtx.ecx; break;
                case 2: u32EffAddr = pVCpu->cpum.GstCtx.edx; break;
                case 3: u32EffAddr = pVCpu->cpum.GstCtx.ebx; break;
                case 4: u32EffAddr = 0; /*none */ break;
                case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; break;
                case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
                case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
            }
            u32EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

            /* add base */
            switch (bSib & X86_SIB_BASE_MASK)
            {
                case 0: u32EffAddr += pVCpu->cpum.GstCtx.eax; break;
                case 1: u32EffAddr += pVCpu->cpum.GstCtx.ecx; break;
                case 2: u32EffAddr += pVCpu->cpum.GstCtx.edx; break;
                case 3: u32EffAddr += pVCpu->cpum.GstCtx.ebx; break;
                case 4: u32EffAddr += pVCpu->cpum.GstCtx.esp; break;
                case 5:
                    if ((bRm & X86_MODRM_MOD_MASK) != 0)
                        u32EffAddr += pVCpu->cpum.GstCtx.ebp;
                    else
                        u32EffAddr += u32Disp;
                    break;
                case 6: u32EffAddr += pVCpu->cpum.GstCtx.esi; break;
                case 7: u32EffAddr += pVCpu->cpum.GstCtx.edi; break;
            }
            break;
        }
        case 5: u32EffAddr = pVCpu->cpum.GstCtx.ebp; break;
        case 6: u32EffAddr = pVCpu->cpum.GstCtx.esi; break;
        case 7: u32EffAddr = pVCpu->cpum.GstCtx.edi; break;
    }

    /* Get and add the displacement. */
    switch ((bRm >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
    {
        case 0: break;
        case 1: u32EffAddr += (int8_t)u32Disp; break;
        case 2: u32EffAddr += u32Disp; break;
        default: AssertFailed();
    }

    Log5(("iemOpHlpCalcRmEffAddrThreadedAddr32: EffAddr=%#010RGv\n", (RTGCPTR)u32EffAddr));
    return u32EffAddr;
}


/**
 * Calculates the effective address of a ModR/M memory operand.
 *
 * Meant to be used via IEM_MC_CALC_RM_EFF_ADDR_THREADED_ADDR64.
 *
 * @returns The effective address.
 * @param   pVCpu               The cross context virtual CPU structure of the calling thread.
 * @param   bRmEx               The ModRM byte but with bit 3 set to REX.B and
 *                              bit 4 to REX.X.  The two bits are part of the
 *                              REG sub-field, which isn't needed in this
 *                              function.
 * @param   bSib                The SIB byte, if any.
 * @param   u32Disp             The displacement byte/word/dword, if any.
 * @param   cbInstr             The size of the fully decoded instruction. Used
 *                              for RIP relative addressing.
 * @todo combine cbInstr and cbImm!
 */
static RTGCPTR iemOpHlpCalcRmEffAddrThreadedAddr64(PVMCPUCC pVCpu, uint8_t bRmEx, uint8_t bSib,
                                                   uint32_t u32Disp, uint8_t cbInstr) RT_NOEXCEPT
{
    Log5(("iemOpHlpCalcRmEffAddrThreadedAddr64: bRmEx=%#x\n", bRmEx));
    Assert(pVCpu->iem.s.enmCpuMode == IEMMODE_64BIT);

    uint64_t u64EffAddr;

    /* Handle the rip+disp32 form with no registers first. */
    if ((bRmEx & (X86_MODRM_MOD_MASK | X86_MODRM_RM_MASK)) == 5)
    {
        u64EffAddr = (int32_t)u32Disp;
        u64EffAddr += pVCpu->cpum.GstCtx.rip + cbInstr;
    }
    else
    {
        /* Get the register (or SIB) value. */
        switch (bRmEx & (X86_MODRM_RM_MASK | 0x8)) /* bRmEx[bit 3] = REX.B */
        {
            case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
            case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
            case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
            case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
            case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; break;
            case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
            case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
            case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
            case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
            case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
            case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
            case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
            case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
            case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
            /* SIB */
            case 4:
            case 12:
            {
                /* Get the index and scale it. */
                switch (((bSib >> X86_SIB_INDEX_SHIFT) & X86_SIB_INDEX_SMASK) | ((bRmEx & 0x10) >> 1)) /* bRmEx[bit 4] = REX.X */
                {
                    case  0: u64EffAddr = pVCpu->cpum.GstCtx.rax; break;
                    case  1: u64EffAddr = pVCpu->cpum.GstCtx.rcx; break;
                    case  2: u64EffAddr = pVCpu->cpum.GstCtx.rdx; break;
                    case  3: u64EffAddr = pVCpu->cpum.GstCtx.rbx; break;
                    case  4: u64EffAddr = 0; /*none */ break;
                    case  5: u64EffAddr = pVCpu->cpum.GstCtx.rbp; break;
                    case  6: u64EffAddr = pVCpu->cpum.GstCtx.rsi; break;
                    case  7: u64EffAddr = pVCpu->cpum.GstCtx.rdi; break;
                    case  8: u64EffAddr = pVCpu->cpum.GstCtx.r8;  break;
                    case  9: u64EffAddr = pVCpu->cpum.GstCtx.r9;  break;
                    case 10: u64EffAddr = pVCpu->cpum.GstCtx.r10; break;
                    case 11: u64EffAddr = pVCpu->cpum.GstCtx.r11; break;
                    case 12: u64EffAddr = pVCpu->cpum.GstCtx.r12; break;
                    case 13: u64EffAddr = pVCpu->cpum.GstCtx.r13; break;
                    case 14: u64EffAddr = pVCpu->cpum.GstCtx.r14; break;
                    case 15: u64EffAddr = pVCpu->cpum.GstCtx.r15; break;
                }
                u64EffAddr <<= (bSib >> X86_SIB_SCALE_SHIFT) & X86_SIB_SCALE_SMASK;

                /* add base */
                switch ((bSib & X86_SIB_BASE_MASK) | (bRmEx & 0x8)) /* bRmEx[bit 3] = REX.B */
                {
                    case  0: u64EffAddr += pVCpu->cpum.GstCtx.rax; break;
                    case  1: u64EffAddr += pVCpu->cpum.GstCtx.rcx; break;
                    case  2: u64EffAddr += pVCpu->cpum.GstCtx.rdx; break;
                    case  3: u64EffAddr += pVCpu->cpum.GstCtx.rbx; break;
                    case  4: u64EffAddr += pVCpu->cpum.GstCtx.rsp; break;
                    case  6: u64EffAddr += pVCpu->cpum.GstCtx.rsi; break;
                    case  7: u64EffAddr += pVCpu->cpum.GstCtx.rdi; break;
                    case  8: u64EffAddr += pVCpu->cpum.GstCtx.r8;  break;
                    case  9: u64EffAddr += pVCpu->cpum.GstCtx.r9;  break;
                    case 10: u64EffAddr += pVCpu->cpum.GstCtx.r10; break;
                    case 11: u64EffAddr += pVCpu->cpum.GstCtx.r11; break;
                    case 12: u64EffAddr += pVCpu->cpum.GstCtx.r12; break;
                    case 14: u64EffAddr += pVCpu->cpum.GstCtx.r14; break;
                    case 15: u64EffAddr += pVCpu->cpum.GstCtx.r15; break;
                    /* complicated encodings */
                    case 5:
                        if ((bRmEx & X86_MODRM_MOD_MASK) != 0)
                            u64EffAddr += pVCpu->cpum.GstCtx.rbp;
                        else
                            u64EffAddr += (int32_t)u32Disp;
                        break;
                    case 13:
                        if ((bRmEx & X86_MODRM_MOD_MASK) != 0)
                            u64EffAddr += pVCpu->cpum.GstCtx.r13;
                        else
                            u64EffAddr += (int32_t)u32Disp;
                        break;
                }
                break;
            }
        }

        /* Get and add the displacement. */
        switch ((bRmEx >> X86_MODRM_MOD_SHIFT) & X86_MODRM_MOD_SMASK)
        {
            case 0: break;
            case 1: u64EffAddr += (int8_t)u32Disp; break;
            case 2: u64EffAddr += (int32_t)u32Disp; break;
            default: AssertFailed();
        }
    }

    Log5(("iemOpHlpCalcRmEffAddrThreadedAddr64: EffAddr=%#010RGv\n", u64EffAddr));
    return u64EffAddr;
}



/*
 * The threaded functions.
 */
#include "IEMThreadedFunctions.cpp.h"

