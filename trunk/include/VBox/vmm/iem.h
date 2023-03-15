/** @file
 * IEM - Interpreted Execution Manager.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_vmm_iem_h
#define VBOX_INCLUDED_vmm_iem_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/trpm.h>
#include <iprt/assert.h>

#ifdef VBOX_VMM_TARGET_ARMV8
# include <VBox/vmm/iem-armv8.h>
#else
# include <VBox/vmm/iem-x86-amd64.h>
#endif


RT_C_DECLS_BEGIN

/** @defgroup grp_iem       The Interpreted Execution Manager API.
 * @ingroup grp_vmm
 * @{
 */

/** @name IEMXCPTRAISEINFO_XXX - Extra info. on a recursive exception situation.
 *
 * This is primarily used by HM for working around a PGM limitation (see
 * @bugref{6607}) and special NMI/IRET handling. In the future, this may be
 * used for diagnostics.
 *
 * @{
 */
typedef uint32_t IEMXCPTRAISEINFO;
/** Pointer to a IEMXCPTINFO type. */
typedef IEMXCPTRAISEINFO *PIEMXCPTRAISEINFO;
/** No addition info. available. */
#define IEMXCPTRAISEINFO_NONE                    RT_BIT_32(0)
/** Delivery of a \#AC caused another \#AC. */
#define IEMXCPTRAISEINFO_AC_AC                   RT_BIT_32(1)
/** Delivery of a \#PF caused another \#PF. */
#define IEMXCPTRAISEINFO_PF_PF                   RT_BIT_32(2)
/** Delivery of a \#PF caused some contributory exception. */
#define IEMXCPTRAISEINFO_PF_CONTRIBUTORY_XCPT    RT_BIT_32(3)
/** Delivery of an external interrupt caused an exception. */
#define IEMXCPTRAISEINFO_EXT_INT_XCPT            RT_BIT_32(4)
/** Delivery of an external interrupt caused an \#PF. */
#define IEMXCPTRAISEINFO_EXT_INT_PF              RT_BIT_32(5)
/** Delivery of a software interrupt caused an exception. */
#define IEMXCPTRAISEINFO_SOFT_INT_XCPT           RT_BIT_32(6)
/** Delivery of an NMI caused an exception. */
#define IEMXCPTRAISEINFO_NMI_XCPT                RT_BIT_32(7)
/** Delivery of an NMI caused a \#PF. */
#define IEMXCPTRAISEINFO_NMI_PF                  RT_BIT_32(8)
/** Can re-execute the instruction at CS:RIP. */
#define IEMXCPTRAISEINFO_CAN_REEXEC_INSTR        RT_BIT_32(9)
/** @} */


/** @name IEMXCPTRAISE_XXX - Ways to handle a recursive exception condition.
 * @{ */
typedef enum IEMXCPTRAISE
{
    /** Raise the current (second) exception. */
    IEMXCPTRAISE_CURRENT_XCPT = 0,
    /** Re-raise the previous (first) event (for HM, unused by IEM). */
    IEMXCPTRAISE_PREV_EVENT,
    /** Re-execute instruction at CS:RIP (for HM, unused by IEM). */
    IEMXCPTRAISE_REEXEC_INSTR,
    /** Raise a \#DF exception. */
    IEMXCPTRAISE_DOUBLE_FAULT,
    /** Raise a triple fault. */
    IEMXCPTRAISE_TRIPLE_FAULT,
    /** Cause a CPU hang. */
    IEMXCPTRAISE_CPU_HANG,
    /** Invalid sequence of events. */
    IEMXCPTRAISE_INVALID = 0x7fffffff
} IEMXCPTRAISE;
/** Pointer to a IEMXCPTRAISE type. */
typedef IEMXCPTRAISE *PIEMXCPTRAISE;
/** @} */


/** @name Operand or addressing mode.
 * @{ */
typedef uint8_t IEMMODE;
#define IEMMODE_16BIT 0
#define IEMMODE_32BIT 1
#define IEMMODE_64BIT 2
/** @} */


/** @name IEM_XCPT_FLAGS_XXX - flags for iemRaiseXcptOrInt.
 * @{ */
/** CPU exception. */
#define IEM_XCPT_FLAGS_T_CPU_XCPT       RT_BIT_32(0)
/** External interrupt (from PIC, APIC, whatever). */
#define IEM_XCPT_FLAGS_T_EXT_INT        RT_BIT_32(1)
/** Software interrupt (int or into, not bound).
 * Returns to the following instruction */
#define IEM_XCPT_FLAGS_T_SOFT_INT       RT_BIT_32(2)
/** Takes an error code. */
#define IEM_XCPT_FLAGS_ERR              RT_BIT_32(3)
/** Takes a CR2. */
#define IEM_XCPT_FLAGS_CR2              RT_BIT_32(4)
/** Generated by the breakpoint instruction. */
#define IEM_XCPT_FLAGS_BP_INSTR         RT_BIT_32(5)
/** Generated by a DRx instruction breakpoint and RF should be cleared. */
#define IEM_XCPT_FLAGS_DRx_INSTR_BP     RT_BIT_32(6)
/** Generated by the icebp instruction. */
#define IEM_XCPT_FLAGS_ICEBP_INSTR      RT_BIT_32(7)
/** Generated by the overflow instruction. */
#define IEM_XCPT_FLAGS_OF_INSTR         RT_BIT_32(8)
/** @}  */


/** @name IEM status codes.
 *
 * Not quite sure how this will play out in the end, just aliasing safe status
 * codes for now.
 *
 * @{ */
#define VINF_IEM_RAISED_XCPT    VINF_EM_RESCHEDULE
/** @} */


VMMDECL(VBOXSTRICTRC)       IEMExecOne(PVMCPUCC pVCpu);
VMMDECL(VBOXSTRICTRC)       IEMExecOneEx(PVMCPUCC pVCpu, uint32_t *pcbWritten);
VMMDECL(VBOXSTRICTRC)       IEMExecOneWithPrefetchedByPC(PVMCPUCC pVCpu, uint64_t OpcodeBytesPC,
                                                         const void *pvOpcodeBytes, size_t cbOpcodeBytes);
VMMDECL(VBOXSTRICTRC)       IEMExecOneBypassEx(PVMCPUCC pVCpu, uint32_t *pcbWritten);
VMMDECL(VBOXSTRICTRC)       IEMExecOneBypassWithPrefetchedByPC(PVMCPUCC pVCpu, uint64_t OpcodeBytesPC,
                                                               const void *pvOpcodeBytes, size_t cbOpcodeBytes);
VMMDECL(VBOXSTRICTRC)       IEMExecOneIgnoreLock(PVMCPUCC pVCpu);
VMMDECL(VBOXSTRICTRC)       IEMExecLots(PVMCPUCC pVCpu, uint32_t cMaxInstructions, uint32_t cPollRate, uint32_t *pcInstructions);
/** Statistics returned by IEMExecForExits. */
typedef struct IEMEXECFOREXITSTATS
{
    uint32_t cInstructions;
    uint32_t cExits;
    uint32_t cMaxExitDistance;
    uint32_t cReserved;
} IEMEXECFOREXITSTATS;
/** Pointer to statistics returned by IEMExecForExits. */
typedef IEMEXECFOREXITSTATS *PIEMEXECFOREXITSTATS;
VMMDECL(VBOXSTRICTRC)       IEMExecForExits(PVMCPUCC pVCpu, uint32_t fWillExit, uint32_t cMinInstructions, uint32_t cMaxInstructions,
                                            uint32_t cMaxInstructionsWithoutExits, PIEMEXECFOREXITSTATS pStats);
VMMDECL(VBOXSTRICTRC)       IEMInjectTrpmEvent(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  IEMInjectTrap(PVMCPUCC pVCpu, uint8_t u8TrapNo, TRPMEVENT enmType, uint16_t uErrCode, RTGCPTR uCr2,
                                          uint8_t cbInstr);

VMM_INT_DECL(int)           IEMBreakpointSet(PVM pVM, RTGCPTR GCPtrBp);
VMM_INT_DECL(int)           IEMBreakpointClear(PVM pVM, RTGCPTR GCPtrBp);

VMM_INT_DECL(void)          IEMTlbInvalidateAll(PVMCPUCC pVCpu);
VMM_INT_DECL(void)          IEMTlbInvalidatePage(PVMCPUCC pVCpu, RTGCPTR GCPtr);
VMM_INT_DECL(void)          IEMTlbInvalidateAllPhysical(PVMCPUCC pVCpu);
VMM_INT_DECL(void)          IEMTlbInvalidateAllPhysicalAllCpus(PVMCC pVM, VMCPUID idCpuCaller);
VMM_INT_DECL(bool)          IEMGetCurrentXcpt(PVMCPUCC pVCpu, uint8_t *puVector, uint32_t *pfFlags, uint32_t *puErr,
                                              uint64_t *puCr2);
VMM_INT_DECL(IEMXCPTRAISE)  IEMEvaluateRecursiveXcpt(PVMCPUCC pVCpu, uint32_t fPrevFlags, uint8_t uPrevVector, uint32_t fCurFlags,
                                                     uint8_t uCurVector, PIEMXCPTRAISEINFO pXcptRaiseInfo);

/** @defgroup grp_iem_r3     The IEM Host Context Ring-3 API.
 * @{
 */
VMMR3DECL(int)      IEMR3Init(PVM pVM);
VMMR3DECL(int)      IEMR3Term(PVM pVM);
VMMR3DECL(void)     IEMR3Relocate(PVM pVM);
VMMR3_INT_DECL(VBOXSTRICTRC) IEMR3ProcessForceFlag(PVM pVM, PVMCPUCC pVCpu, VBOXSTRICTRC rcStrict);
/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_iem_h */

