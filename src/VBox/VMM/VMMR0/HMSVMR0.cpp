/* $Id$ */
/** @file
 * HM SVM (AMD-V) - Host Context Ring-0.
 */

/*
 * Copyright (C) 2013 Oracle Corporation
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

#ifdef DEBUG_ramshankar
# define HMSVM_ALWAYS_TRAP_ALL_XCPTS
# define HMSVM_ALWAYS_TRAP_PF
#endif


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifdef VBOX_WITH_STATISTICS
# define HMSVM_EXITCODE_STAM_COUNTER_INC(u64ExitCode) do { \
        if ((u64ExitCode) == SVM_EXIT_NPF) \
            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitReasonNpf); \
        else \
            STAM_COUNTER_INC(&pVCpu->hm.s.paStatExitReasonR0[(u64ExitCode) & MASK_EXITREASON_STAT]); \
        } while (0)
#else
# define HMSVM_EXITCODE_STAM_COUNTER_INC(u64ExitCode) do { } while (0)
#endif

/** @name Segment attribute conversion between CPU and AMD-V VMCB format.
 *
 * The CPU format of the segment attribute is described in X86DESCATTRBITS
 * which is 16-bits (i.e. includes 4 bits of the segment limit).
 *
 * The AMD-V VMCB format the segment attribute is compact 12-bits (strictly
 * only the attribute bits and nothing else). Upper 4-bits are unused.
 *
 * @{ */
#define HMSVM_CPU_2_VMCB_SEG_ATTR(a)       (a & 0xff) | ((a & 0xf000) >> 4)
#define HMSVM_VMCB_2_CPU_SEG_ATTR(a)       (a & 0xff) | ((a & 0x0f00) << 4)
/** @} */

/** @name Macros for loading, storing segment registers to/from the VMCB.
 *  @{ */
#define HMSVM_LOAD_SEG_REG(REG, reg) \
    do \
    { \
        Assert(pCtx->reg.fFlags & CPUMSELREG_FLAGS_VALID); \
        Assert(pCtx->reg.ValidSel == pCtx->reg.Sel); \
        pVmcb->guest.REG.u16Sel     = pCtx->reg.Sel; \
        pVmcb->guest.REG.u32Limit   = pCtx->reg.u32Limit; \
        pVmcb->guest.REG.u64Base    = pCtx->reg.u64Base; \
        pVmcb->guest.REG.u16Attr    = HMSVM_CPU_2_VMCB_SEG_ATTR(pCtx->reg.Attr.u); \
    } while (0)

#define HMSVM_SAVE_SEG_REG(REG, reg) \
    do \
    { \
        pCtx->reg.Sel       = pVmcb->guest.REG.u16Sel; \
        pCtx->reg.ValidSel  = pVmcb->guest.REG.u16Sel; \
        pCtx->reg.fFlags    = CPUMSELREG_FLAGS_VALID; \
        pCtx->reg.u32Limit  = pVmcb->guest.REG.u32Limit; \
        pCtx->reg.u64Base   = pVmcb->guest.REG.u64Base; \
        pCtx->reg.Attr.u    = HMSVM_VMCB_2_CPU_SEG_ATTR(pVmcb->guest.REG.u16Attr); \
    } while (0)
/** @} */

/** @name VMCB Clean Bits.
 *
 * These flags are used for VMCB-state caching. A set VMCB Clean Bit indicates
 * AMD-V doesn't need to reload the corresponding value(s) from the VMCB in
 * memory.
 *
 * @{ */
/** All intercepts vectors, TSC offset, PAUSE filter counter. */
#define HMSVM_VMCB_CLEAN_INTERCEPTS             RT_BIT(0)
/** I/O permission bitmap, MSR permission bitmap. */
#define HMSVM_VMCB_CLEAN_IOPM_MSRPM             RT_BIT(1)
/** ASID.  */
#define HMSVM_VMCB_CLEAN_ASID                   RT_BIT(2)
/** TRP: V_TPR, V_IRQ, V_INTR_PRIO, V_IGN_TPR, V_INTR_MASKING,
V_INTR_VECTOR. */
#define HMSVM_VMCB_CLEAN_TPR                    RT_BIT(3)
/** Nested Paging: Nested CR3 (nCR3), PAT. */
#define HMSVM_VMCB_CLEAN_NP                     RT_BIT(4)
/** Control registers (CR0, CR3, CR4, EFER). */
#define HMSVM_VMCB_CLEAN_CRX                    RT_BIT(5)
/** Debug registers (DR6, DR7). */
#define HMSVM_VMCB_CLEAN_DRX                    RT_BIT(6)
/** GDT, IDT limit and base. */
#define HMSVM_VMCB_CLEAN_DT                     RT_BIT(7)
/** Segment register: CS, SS, DS, ES limit and base. */
#define HMSVM_VMCB_CLEAN_SEG                    RT_BIT(8)
/** CR2.*/
#define HMSVM_VMCB_CLEAN_CR2                    RT_BIT(9)
/** Last-branch record (DbgCtlMsr, br_from, br_to, lastint_from, lastint_to) */
#define HMSVM_VMCB_CLEAN_LBR                    RT_BIT(10)
/** AVIC (AVIC APIC_BAR; AVIC APIC_BACKING_PAGE, AVIC
PHYSICAL_TABLE and AVIC LOGICAL_TABLE Pointers). */
#define HMSVM_VMCB_CLEAN_AVIC                   RT_BIT(11)
/** Mask of all valid VMCB Clean bits. */
#define HMSVM_VMCB_CLEAN_ALL                    (  HMSVM_VMCB_CLEAN_INTERCEPTS
                                                 | HMSVM_VMCB_CLEAN_IOPM_MSRPM
                                                 | HMSVM_VMCB_CLEAN_ASID
                                                 | HMSVM_VMCB_CLEAN_TPR
                                                 | HMSVM_VMCB_CLEAN_NP
                                                 | HMSVM_VMCB_CLEAN_CRX
                                                 | HMSVM_VMCB_CLEAN_DRX
                                                 | HMSVM_VMCB_CLEAN_DT
                                                 | HMSVM_VMCB_CLEAN_SEG
                                                 | HMSVM_VMCB_CLEAN_CR2
                                                 | HMSVM_VMCB_CLEAN_LBR
                                                 | HMSVM_VMCB_CLEAN_AVIC)
/** @} */

/** @name SVM transient.
 *
 * A state structure for holding miscellaneous information across AMD-V
 * VMRUN/#VMEXIT operation, restored after the transition.
 *
 * @{ */
typedef struct SVMTRANSIENT
{
    /** The host's rflags/eflags. */
    RTCCUINTREG     uEFlags;
#if HC_ARCH_BITS == 32
    uint32_t        u32Alignment0;
#endif

    /** The #VMEXIT exit code (the EXITCODE field in the VMCB). */
    uint64_t        u64ExitCode;
} SVMTRANSIENT, *PSVMTRANSIENT;
/** @}  */


/**
 * MSRPM (MSR permission bitmap) read permissions (for guest RDMSR).
 */
typedef enum SVMMSREXITREAD
{
    /** Reading this MSR causes a VM-exit. */
    SVMMSREXIT_INTERCEPT_READ = 0xb,
    /** Reading this MSR does not cause a VM-exit. */
    SVMMSREXIT_PASSTHRU_READ
} SVMMSREXITREAD;

/**
 * MSRPM (MSR permission bitmap) write permissions (for guest WRMSR).
 */
typedef enum SVMMSREXITWRITE
{
    /** Writing to this MSR causes a VM-exit. */
    SVMMSREXIT_INTERCEPT_WRITE = 0xd,
    /** Writing to this MSR does not cause a VM-exit. */
    SVMMSREXIT_PASSTHRU_WRITE
} SVMMSREXITWRITE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void hmR0SvmSetMsrPermission(PVMCPU pVCpu, unsigned uMsr, SVMMSREXITREAD enmRead, SVMMSREXITWRITE enmWrite);

DECLINLINE(int) hmR0SvmHandleExit(PVMCPU pVCpu, PCPUMCTX pMixedCtx, PSVMTRANSIENT pSvmTransient);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Ring-0 memory object for the IO bitmap. */
RTR0MEMOBJ                  g_hMemObjIOBitmap = NIL_RTR0MEMOBJ;
/** Physical address of the IO bitmap. */
RTHCPHYS                    g_HCPhysIOBitmap  = 0;
/** Virtual address of the IO bitmap. */
R0PTRTYPE(void *)           g_pvIOBitmap      = NULL;


/**
 * Sets up and activates AMD-V on the current CPU.
 *
 * @returns VBox status code.
 * @param   pCpu            Pointer to the CPU info struct.
 * @param   pVM             Pointer to the VM (can be NULL after a resume!).
 * @param   pvCpuPage       Pointer to the global CPU page.
 * @param   HCPhysCpuPage   Physical address of the global CPU page.
 */
VMMR0DECL(int) SVMR0EnableCpu(PHMGLOBLCPUINFO pCpu, PVM pVM, void *pvCpuPage, RTHCPHYS HCPhysCpuPage, bool fEnabledByHost)
{
    AssertReturn(!fEnabledByHost, VERR_INVALID_PARAMETER);
    AssertReturn(   HCPhysCpuPage
                 && HCPhysCpuPage != NIL_RTHCPHYS, VERR_INVALID_PARAMETER);
    AssertReturn(pvCpuPage, VERR_INVALID_PARAMETER);

    /*
     * We must turn on AMD-V and setup the host state physical address, as those MSRs are per CPU.
     */
    uint64_t u64HostEfer = ASMRdMsr(MSR_K6_EFER);
    if (u64HostEfer & MSR_K6_EFER_SVME)
    {
        /* If the VBOX_HWVIRTEX_IGNORE_SVM_IN_USE is active, then we blindly use AMD-V. */
        if (   pVM
            && pVM->hm.s.svm.fIgnoreInUseError)
        {
            pCpu->fIgnoreAMDVInUseError = true;
        }

        if (!pCpu->fIgnoreAMDVInUseError)
            return VERR_SVM_IN_USE;
    }

    /* Turn on AMD-V in the EFER MSR. */
    ASMWrMsr(MSR_K6_EFER, u64HostEfer | MSR_K6_EFER_SVME);

    /* Write the physical page address where the CPU will store the host state while executing the VM. */
    ASMWrMsr(MSR_K8_VM_HSAVE_PA, HCPhysCpuPage);

    /*
     * Theoretically, other hypervisors may have used ASIDs, ideally we should flush all non-zero ASIDs
     * when enabling SVM. AMD doesn't have an SVM instruction to flush all ASIDs (flushing is done
     * upon VMRUN). Therefore, just set the fFlushAsidBeforeUse flag which instructs hmR0SvmSetupTLB()
     * to flush the TLB with before using a new ASID.
     */
    pCpu->fFlushAsidBeforeUse = true;

    /*
     * Ensure each VCPU scheduled on this CPU gets a new VPID on resume. See @bugref{6255}.
     */
    ++pCpu->cTlbFlushes;

    return VINF_SUCCESS;
}


/**
 * Deactivates AMD-V on the current CPU.
 *
 * @returns VBox status code.
 * @param   pCpu            Pointer to the CPU info struct.
 * @param   pvCpuPage       Pointer to the global CPU page.
 * @param   HCPhysCpuPage   Physical address of the global CPU page.
 */
VMMR0DECL(int) SVMR0DisableCpu(PHMGLOBLCPUINFO pCpu, void *pvCpuPage, RTHCPHYS HCPhysCpuPage)
{
    AssertReturn(   HCPhysCpuPage
                 && HCPhysCpuPage != NIL_RTHCPHYS, VERR_INVALID_PARAMETER);
    AssertReturn(pvCpuPage, VERR_INVALID_PARAMETER);
    NOREF(pCpu);

    /* Turn off AMD-V in the EFER MSR if AMD-V is active. */
    uint64_t u64HostEfer = ASMRdMsr(MSR_K6_EFER);
    if (u64HostEfer & MSR_K6_EFER_SVME)
    {
        ASMWrMsr(MSR_K6_EFER, u64HostEfer & ~MSR_K6_EFER_SVME);

        /* Invalidate host state physical address. */
        ASMWrMsr(MSR_K8_VM_HSAVE_PA, 0);
    }

    return VINF_SUCCESS;
}


/**
 * Does global AMD-V initialization (called during module initialization).
 *
 * @returns VBox status code.
 */
VMMR0DECL(int) SVMR0GlobalInit(void)
{
    /*
     * Allocate 12 KB for the IO bitmap. Since this is non-optional and we always intercept all IO accesses, it's done
     * once globally here instead of per-VM.
     */
    int rc = RTR0MemObjAllocCont(&g_hMemObjIOBitmap, 3 << PAGE_SHIFT, false /* fExecutable */);
    if (RT_FAILURE(rc))
        return rc;

    g_pvIOBitmap     = RTR0MemObjAddress(g_hMemObjIOBitmap);
    g_HCPhysIOBitmap = RTR0MemObjGetPagePhysAddr(g_hMemObjIOBitmap, 0 /* iPage */);

    /* Set all bits to intercept all IO accesses. */
    ASMMemFill32(pVM->hm.s.svm.pvIOBitmap, 3 << PAGE_SHIFT, UINT32_C(0xffffffff));
}


/**
 * Does global VT-x termination (called during module termination).
 */
VMMR0DECL(void) SVMR0GlobalTerm(void)
{
    if (g_hMemObjIOBitmap != NIL_RTR0MEMOBJ)
    {
        RTR0MemObjFree(pVM->hm.s.svm.hMemObjIOBitmap, false /* fFreeMappings */);
        g_pvIOBitmap      = NULL;
        g_HCPhysIOBitmap  = 0;
        g_hMemObjIOBitmap = NIL_RTR0MEMOBJ;
    }
}


/**
 * Frees any allocated per-VCPU structures for a VM.
 *
 * @param   pVM     Pointer to the VM.
 */
DECLINLINE(void) hmR0SvmFreeStructs(PVM pVM)
{
    for (uint32_t i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        AssertPtr(pVCpu);

        if (pVCpu->hm.s.svm.hMemObjVmcbHost != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pVCpu->hm.s.svm.hMemObjVmcbHost, false);
            pVCpu->hm.s.svm.pvVmcbHost       = 0;
            pVCpu->hm.s.svm.HCPhysVmcbHost   = 0;
            pVCpu->hm.s.svm.hMemObjVmcbHost  = NIL_RTR0MEMOBJ;
        }

        if (pVCpu->hm.s.svm.hMemObjVmcb != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pVCpu->hm.s.svm.hMemObjVmcb, false);
            pVCpu->hm.s.svm.pvVmcb           = 0;
            pVCpu->hm.s.svm.HCPhysVmcb       = 0;
            pVCpu->hm.s.svm.hMemObjVmcb      = NIL_RTR0MEMOBJ;
        }

        if (pVCpu->hm.s.svm.hMemObjMsrBitmap != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pVCpu->hm.s.svm.hMemObjMsrBitmap, false);
            pVCpu->hm.s.svm.pvMsrBitmap      = 0;
            pVCpu->hm.s.svm.HCPhysMsrBitmap  = 0;
            pVCpu->hm.s.svm.hMemObjMsrBitmap = NIL_RTR0MEMOBJ;
        }
    }
}


/**
 * Does per-VM AMD-V initialization.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 */
VMMR0DECL(int) SVMR0InitVM(PVM pVM)
{
    int rc = VERR_INTERNAL_ERROR_5;

    /*
     * Check for an AMD CPU erratum which requires us to flush the TLB before every world-switch.
     */
    uint32_t u32Family;
    uint32_t u32Model;
    uint32_t u32Stepping;
    if (HMAmdIsSubjectToErratum170(&u32Family, &u32Model, &u32Stepping))
    {
        Log4(("SVMR0InitVM: AMD cpu with erratum 170 family %#x model %#x stepping %#x\n", u32Family, u32Model, u32Stepping));
        pVM->hm.s.svm.fAlwaysFlushTLB = true;
    }

    /*
     * Initialize the R0 memory objects up-front so we can properly cleanup on allocation failures.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        pVCpu->hm.s.svm.hMemObjVmcbHost  = NIL_RTR0MEMOBJ;
        pVCpu->hm.s.svm.hMemObjVmcb      = NIL_RTR0MEMOBJ;
        pVCpu->hm.s.svm.hMemObjMsrBitmap = NIL_RTR0MEMOBJ;
    }

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        /*
         * Allocate one page for the host-context VM control block (VMCB). This is used for additional host-state (such as
         * FS, GS, Kernel GS Base, etc.) apart from the host-state save area specified in MSR_K8_VM_HSAVE_PA.
         */
        rc = RTR0MemObjAllocCont(&pVCpu->hm.s.svm.hMemObjVmcbHost, 1 << PAGE_SHIFT, false /* fExecutable */);
        if (RT_FAILURE(rc))
            goto failure_cleanup;

        pVCpu->hm.s.svm.pvVmcbHost     = RTR0MemObjAddress(pVCpu->hm.s.svm.hMemObjVmcbHost);
        pVCpu->hm.s.svm.HCPhysVmcbHost = RTR0MemObjGetPagePhysAddr(pVCpu->hm.s.svm.hMemObjVmcbHost, 0 /* iPage */);
        Assert(pVCpu->hm.s.svm.HCPhysVmcbHost < _4G);
        ASMMemZeroPage(pVCpu->hm.s.svm.pvVmcbHost);

        /*
         * Allocate one page for the guest-state VMCB.
         */
        rc = RTR0MemObjAllocCont(&pVCpu->hm.s.svm.hMemObjVmcb, 1 << PAGE_SHIFT, false /* fExecutable */);
        if (RT_FAILURE(rc))
            goto failure_cleanup;

        pVCpu->hm.s.svm.pvVmcb          = RTR0MemObjAddress(pVCpu->hm.s.svm.hMemObjVmcb);
        pVCpu->hm.s.svm.HCPhysVmcb      = RTR0MemObjGetPagePhysAddr(pVCpu->hm.s.svm.hMemObjVmcb, 0 /* iPage */);
        Assert(pVCpu->hm.s.svm.HCPhysVmcb < _4G);
        ASMMemZeroPage(pVCpu->hm.s.svm.pvVmcb);

        /*
         * Allocate two pages (8 KB) for the MSR permission bitmap. There doesn't seem to be a way to convince
         * SVM to not require one.
         */
        rc = RTR0MemObjAllocCont(&pVCpu->hm.s.svm.hMemObjMsrBitmap, 2 << PAGE_SHIFT, false /* fExecutable */);
        if (RT_FAILURE(rc))
            failure_cleanup;

        pVCpu->hm.s.svm.pvMsrBitmap     = RTR0MemObjAddress(pVCpu->hm.s.svm.hMemObjMsrBitmap);
        pVCpu->hm.s.svm.HCPhysMsrBitmap = RTR0MemObjGetPagePhysAddr(pVCpu->hm.s.svm.hMemObjMsrBitmap, 0 /* iPage */);
        /* Set all bits to intercept all MSR accesses (changed later on). */
        ASMMemFill32(pVCpu->hm.s.svm.pvMsrBitmap, 2 << PAGE_SHIFT, 0xffffffff);
    }

    return VINF_SUCCESS;

failure_cleanup:
    hmR0SvmFreeVMStructs(pVM);
    return rc;
}


/**
 * Does per-VM AMD-V termination.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 */
VMMR0DECL(int) SVMR0TermVM(PVM pVM)
{
    hmR0SvmFreeVMStructs(pVM);
    return VINF_SUCCESS;
}


/**
 * Sets the permission bits for the specified MSR in the MSRPM.
 *
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   uMsr       The MSR.
 * @param   fRead       Whether reading is allowed.
 * @param   fWrite      Whether writing is allowed.
 */
static void hmR0SvmSetMsrPermission(PVMCPU pVCpu, uint32_t uMsr, SVMMSREXITREAD enmRead, SVMMSREXITWRITE enmWrite)
{
    unsigned ulBit;
    uint8_t *pbMsrBitmap = (uint8_t *)pVCpu->hm.s.svm.pvMsrBitmap;

    /*
     * Layout:
     * Byte offset       MSR range
     * 0x000  - 0x7ff    0x00000000 - 0x00001fff
     * 0x800  - 0xfff    0xc0000000 - 0xc0001fff
     * 0x1000 - 0x17ff   0xc0010000 - 0xc0011fff
     * 0x1800 - 0x1fff           Reserved
     */
    if (uMsr <= 0x00001FFF)
    {
        /* Pentium-compatible MSRs. */
        ulBit    = uMsr * 2;
    }
    else if (   uMsr >= 0xC0000000
             && uMsr <= 0xC0001FFF)
    {
        /* AMD Sixth Generation x86 Processor MSRs. */
        ulBit = (uMsr - 0xC0000000) * 2;
        pbMsrBitmap += 0x800;
    }
    else if (   uMsr >= 0xC0010000
             && uMsr <= 0xC0011FFF)
    {
        /* AMD Seventh and Eighth Generation Processor MSRs. */
        ulBit = (uMsr - 0xC0001000) * 2;
        pbMsrBitmap += 0x1000;
    }
    else
    {
        AssertFailed();
        return;
    }

    Assert(ulBit < 0x3fff /* 16 * 1024 - 1 */);
    if (enmRead == SVMMSREXIT_INTERCEPT_READ)
        ASMBitSet(pbMsrBitmap, ulBit);
    else
        ASMBitClear(pbMsrBitmap, ulBit);

    if (enmWrite == SVMMSREXIT_INTERCEPT_WRITE)
        ASMBitSet(pbMsrBitmap, ulBit + 1);
    else
        ASMBitClear(pbMsrBitmap, ulBit + 1);

    pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_IOPM_MSRPM;
}


/**
 * Sets up AMD-V for the specified VM.
 * This function is only called once per-VM during initalization.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 */
VMMR0DECL(int) SVMR0SetupVM(PVM pVM)
{
    int rc = VINF_SUCCESS;

    AssertReturn(pVM, VERR_INVALID_PARAMETER);
    Assert(pVM->hm.s.svm.fSupported);

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU   pVCpu = &pVM->aCpus[i];
        PSVMVMCB pVmcb = (PSVMVMCB)pVM->aCpus[i].hm.s.svm.pvVmcb;

        AssertMsgReturn(pVmcb, ("Invalid pVmcb\n"), VERR_SVM_INVALID_PVMCB);

        /* Trap exceptions unconditionally (debug purposes). */
#ifdef HMSVM_ALWAYS_TRAP_PF
        pVmcb->ctrl.u32InterceptException |=   RT_BIT(X86_XCPT_PF);
#endif
#ifdef HMSVM_ALWAYS_TRAP_ALL_XCPTS
        pVmcb->ctrl.u32InterceptException |=   RT_BIT(X86_XCPT_BP)
                                             | RT_BIT(X86_XCPT_DB)
                                             | RT_BIT(X86_XCPT_DE)
                                             | RT_BIT(X86_XCPT_NM)
                                             | RT_BIT(X86_XCPT_UD)
                                             | RT_BIT(X86_XCPT_NP)
                                             | RT_BIT(X86_XCPT_SS)
                                             | RT_BIT(X86_XCPT_GP)
                                             | RT_BIT(X86_XCPT_PF)
                                             | RT_BIT(X86_XCPT_MF);
#endif

        /* Set up unconditional intercepts and conditions. */
        pVmcb->ctrl.u32InterceptCtrl1 =   SVM_CTRL1_INTERCEPT_INTR          /* External interrupt causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_VINTR         /* When guest enables interrupts cause a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_NMI           /* Non-Maskable Interrupts causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_SMI           /* System Management Interrupt cause a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_INIT          /* INIT signal causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_RDPMC         /* RDPMC causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_CPUID         /* CPUID causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_RSM           /* RSM causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_HLT           /* HLT causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_INOUT_BITMAP  /* Use the IOPM to cause IOIO VM-exits. */
                                        | SVM_CTRL1_INTERCEPT_MSR_SHADOW    /* MSR access not covered by MSRPM causes a VM-exit.*/
                                        | SVM_CTRL1_INTERCEPT_INVLPGA       /* INVLPGA causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_SHUTDOWN      /* Shutdown events causes a VM-exit. */
                                        | SVM_CTRL1_INTERCEPT_FERR_FREEZE;  /* Intercept "freezing" during legacy FPU handling. */

        pVmcb->ctrl.u32InterceptCtrl2 =   SVM_CTRL2_INTERCEPT_VMRUN         /* VMRUN causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_VMMCALL       /* VMMCALL causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_VMLOAD        /* VMLOAD causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_VMSAVE        /* VMSAVE causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_STGI          /* STGI causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_CLGI          /* CLGI causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_SKINIT        /* SKINIT causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_WBINVD        /* WBINVD causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_MONITOR       /* MONITOR causes a VM-exit. */
                                        | SVM_CTRL2_INTERCEPT_MWAIT_UNCOND; /* MWAIT causes a VM-exit. */

        /* CR0, CR4 reads must be intercepted, our shadow values are not necessarily the same as the guest's. */
        pVmcb->ctrl.u16InterceptRdCRx = RT_BIT(0) | RT_BIT(4);

        /* CR0, CR4 writes must be intercepted for the same reasons as above. */
        pVmcb->ctrl.u16InterceptWrCRx = RT_BIT(0) | RT_BIT(4);

        /* Intercept all DRx reads and writes by default. Changed later on. */
        pVmcb->ctrl.u16InterceptRdDRx = 0xffff;
        pVmcb->ctrl.u16InterceptWrDRx = 0xffff;

        /* Virtualize masking of INTR interrupts. (reads/writes from/to CR8 go to the V_TPR register) */
        pVmcb->ctrl.IntCtrl.n.u1VIrqMasking = 1;

        /* Ignore the priority in the TPR; we take into account the guest TPR anyway while delivering interrupts. */
        pVmcb->ctrl.IntCtrl.n.u1IgnoreTPR   = 1;

        /* Set IO and MSR bitmap permission bitmap physical addresses. */
        pVmcb->ctrl.u64IOPMPhysAddr  = g_HCPhysIOBitmap;
        pVmcb->ctrl.u64MSRPMPhysAddr = pVCpu->hm.s.svm.HCPhysMsrBitmap;

        /* No LBR virtualization. */
        pVmcb->ctrl.u64LBRVirt = 0;

        /* Initially set all VMCB clean bits to 0 indicating that everything should be loaded from memory. */
        pVmcb->ctrl.u64VmcbCleanBits = 0;

        /* The guest ASID MBNZ, set it to 1. The host uses 0. */
        pVmcb->ctrl.TLBCtrl.n.u32ASID = 1;

        /*
         * Setup the PAT MSR (applicable for Nested Paging only).
         * The default value should be 0x0007040600070406ULL, but we want to treat all guest memory as WB,
         * so choose type 6 for all PAT slots.
         */
        pVmcb->guest.u64GPAT = UINT64_C(0x0006060606060606);

        /* Without Nested Paging, we need additionally intercepts. */
        if (!pVM->hm.s.fNestedPaging)
        {
            /* CR3 reads/writes must be intercepted; our shadow values differ from the guest values. */
            pVmcb->ctrl.u16InterceptRdCRx |= RT_BIT(3);
            pVmcb->ctrl.u16InterceptWrCRx |= RT_BIT(3);

            /* Intercept INVLPG and task switches (may change CR3, EFLAGS, LDT). */
            pVmcb->ctrl.u32InterceptCtrl1 |=   SVM_CTRL1_INTERCEPT_INVLPG
                                             | SVM_CTRL1_INTERCEPT_TASK_SWITCH;

            /* Page faults must be intercepted to implement shadow paging. */
            pVmcb->ctrl.u32InterceptException |= RT_BIT(X86_XCPT_PF);
        }

        /*
         * The following MSRs are saved/restored automatically during the world-switch.
         * Don't intercept guest read/write accesses to these MSRs.
         */
        hmR0SvmSetMsrPermission(pVCpu, MSR_K8_LSTAR, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_K8_CSTAR, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_K6_STAR, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_K8_SF_MASK, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_K8_FS_BASE, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_K8_GS_BASE, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_K8_KERNEL_GS_BASE, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_IA32_SYSENTER_CS, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_IA32_SYSENTER_ESP, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVCpu, MSR_IA32_SYSENTER_EIP, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
    }

    return rc;
}


/**
 * Flushes the appropriate tagged-TLB entries.
 *
 * @param    pVM        Pointer to the VM.
 * @param    pVCpu      Pointer to the VMCPU.
 */
static void hmR0SvmFlushTaggedTlb(PVMCPU pVCpu)
{
    PVM pVM              = pVCpu->CTX_SUFF(pVM);
    PSVMVMCB pVmcb       = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;
    PHMGLOBLCPUINFO pCpu = HMR0GetCurrentCpu();

    /*
     * Force a TLB flush for the first world switch if the current CPU differs from the one we ran on last.
     * This can happen both for start & resume due to long jumps back to ring-3.
     * If the TLB flush count changed, another VM (VCPU rather) has hit the ASID limit while flushing the TLB,
     * so we cannot reuse the ASIDs without flushing.
     */
    bool fNewAsid = false;
    if (   pVCpu->hm.s.idLastCpu   != pCpu->idCpu
        || pVCpu->hm.s.cTlbFlushes != pCpu->cTlbFlushes)
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushTlbWorldSwitch);
        pVCpu->hm.s.fForceTLBFlush = true;
        fNewAsid = true;
    }

    /* Set TLB flush state as checked until we return from the world switch. */
    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, true);

    /* Check for explicit TLB shootdowns. */
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_TLB_FLUSH))
    {
        pVCpu->hm.s.fForceTLBFlush = true;
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushTlb);
    }

    pVCpu->hm.s.idLastCpu = pCpu->idCpu;
    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_NOTHING;

    if (pVM->hm.s.svm.fAlwaysFlushTLB)
    {
        /*
         * This is the AMD erratum 170. We need to flush the entire TLB for each world switch. Sad.
         */
        pCpu->uCurrentAsid               = 1;
        pVCpu->hm.s.uCurrentAsid         = 1;
        pVCpu->hm.s.cTlbFlushes          = pCpu->cTlbFlushes;
        pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;
    }
    else if (pVCpu->hm.s.fForceTLBFlush)
    {
        if (fNewAsid)
        {
            ++pCpu->uCurrentAsid;
            bool fHitASIDLimit = false;
            if (pCpu->uCurrentAsid >= pVM->hm.s.uMaxAsid)
            {
                pCpu->uCurrentAsid        = 1;      /* Wraparound at 1; host uses 0 */
                pCpu->cTlbFlushes++;                /* All VCPUs that run on this host CPU must use a new VPID. */
                fHitASIDLimit             = true;

                if (pVM->hm.s.svm.u32Features & AMD_CPUID_SVM_FEATURE_EDX_FLUSH_BY_ASID)
                {
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_SINGLE_CONTEXT;
                    pCpu->fFlushAsidBeforeUse = true;
                }
                else
                {
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;
                    pCpu->fFlushAsidBeforeUse = false;
                }
            }

            if (   !fHitASIDLimit
                && pCpu->fFlushAsidBeforeUse)
            {
                if (pVM->hm.s.svm.u32Features & AMD_CPUID_SVM_FEATURE_EDX_FLUSH_BY_ASID)
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_SINGLE_CONTEXT;
                else
                {
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;
                    pCpu->fFlushAsidBeforeUse = false;
                }
            }

            pVCpu->hm.s.uCurrentAsid = pCpu->uCurrentAsid;
            pVCpu->hm.s.cTlbFlushes  = pCpu->cTlbFlushes;
        }
        else
        {
            if (pVM->hm.s.svm.u32Features & AMD_CPUID_SVM_FEATURE_EDX_FLUSH_BY_ASID)
                pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_SINGLE_CONTEXT;
            else
                pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;
        }

        pVCpu->hm.s.fForceTLBFlush = false;
    }
    else
    {
        /** @todo We never set VMCPU_FF_TLB_SHOOTDOWN anywhere so this path should
         *        not be executed. See hmQueueInvlPage() where it is commented
         *        out. Support individual entry flushing someday. */
        if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_TLB_SHOOTDOWN))
        {
            /* Deal with pending TLB shootdown actions which were queued when we were not executing code. */
            STAM_COUNTER_INC(&pVCpu->hm.s.StatTlbShootdown);
            for (uint32_t i = 0; i < pVCpu->hm.s.TlbShootdown.cPages; i++)
                SVMR0InvlpgA(pVCpu->hm.s.TlbShootdown.aPages[i], pVmcb->ctrl.TLBCtrl.n.u32ASID);
        }
    }

    pVCpu->hm.s.TlbShootdown.cPages = 0;
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TLB_SHOOTDOWN);

    /* Update VMCB with the ASID. */
    if (pVmcb->ctrl.TLBCtrl.n.u32ASID != pVCpu->hm.s.uCurrentAsid)
    {
        pVmcb->ctrl.TLBCtrl.n.u32ASID = pVCpu->hm.s.uCurrentAsid;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_ASID;
    }

    AssertMsg(pVCpu->hm.s.cTlbFlushes == pCpu->cTlbFlushes,
              ("Flush count mismatch for cpu %d (%x vs %x)\n", pCpu->idCpu, pVCpu->hm.s.cTlbFlushes, pCpu->cTlbFlushes));
    AssertMsg(pCpu->uCurrentAsid >= 1 && pCpu->uCurrentAsid < pVM->hm.s.uMaxAsid,
              ("cpu%d uCurrentAsid = %x\n", pCpu->idCpu, pCpu->uCurrentAsid));
    AssertMsg(pVCpu->hm.s.uCurrentAsid >= 1 && pVCpu->hm.s.uCurrentAsid < pVM->hm.s.uMaxAsid,
              ("cpu%d VM uCurrentAsid = %x\n", pCpu->idCpu, pVCpu->hm.s.uCurrentAsid));

#ifdef VBOX_WITH_STATISTICS
    if (pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_NOTHING)
        STAM_COUNTER_INC(&pVCpu->hm.s.StatNoFlushTlbWorldSwitch);
    else if (   pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_SINGLE_CONTEXT
             || pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_SINGLE_CONTEXT_RETAIN_GLOBALS)
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushAsid);
    }
    else
        Assert(pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_ENTIRE)
#endif
}


/** @name 64-bit guest on 32-bit host OS helper functions.
 *
 * The host CPU is still 64-bit capable but the host OS is running in 32-bit
 * mode (code segment, paging). These wrappers/helpers perform the necessary
 * bits for the 32->64 switcher.
 *
 * @{ */
#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS) && !defined(VBOX_WITH_HYBRID_32BIT_KERNEL)
/**
 * Prepares for and executes VMRUN (64-bit guests on a 32-bit host).
 *
 * @returns VBox status code.
 * @param   HCPhysVmcbHost  Physical address of host VMCB.
 * @param   HCPhysVmcb      Physical address of the VMCB.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pVM             Pointer to the VM.
 * @param   pVCpu           Pointer to the VMCPU.
 */
DECLASM(int) SVMR0VMSwitcherRun64(RTHCPHYS HCPhysVmcbHost, RTHCPHYS HCPhysVmcb, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu)
{
    uint32_t aParam[4];
    aParam[0] = (uint32_t)(HCPhysVmcbHost);             /* Param 1: HCPhysVmcbHost - Lo. */
    aParam[1] = (uint32_t)(HCPhysVmcbHost >> 32);       /* Param 1: HCPhysVmcbHost - Hi. */
    aParam[2] = (uint32_t)(HCPhysVmcb);                 /* Param 2: HCPhysVmcb - Lo. */
    aParam[3] = (uint32_t)(HCPhysVmcb >> 32);           /* Param 2: HCPhysVmcb - Hi. */

    return SVMR0Execute64BitsHandler(pVM, pVCpu, pCtx, HM64ON32OP_SVMRCVMRun64, 4, &aParam[0]);
}


/**
 * Executes the specified VMRUN handler in 64-bit mode.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   enmOp       The operation to perform.
 * @param   cbParam     Number of parameters.
 * @param   paParam     Array of 32-bit parameters.
 */
VMMR0DECL(int) SVMR0Execute64BitsHandler(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, HM64ON32OP enmOp, uint32_t cbParam,
                                         uint32_t *paParam)
{
    AssertReturn(pVM->hm.s.pfnHost32ToGuest64R0, VERR_HM_NO_32_TO_64_SWITCHER);
    Assert(enmOp > HM64ON32OP_INVALID && enmOp < HM64ON32OP_END);

    /* Disable interrupts. */
    RTHCUINTREG uOldEFlags = ASMIntDisableFlags();

#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
    RTCPUID idHostCpu = RTMpCpuId();
    CPUMR0SetLApic(pVM, idHostCpu);
#endif

    CPUMSetHyperESP(pVCpu, VMMGetStackRC(pVCpu));
    CPUMSetHyperEIP(pVCpu, enmOp);
    for (int i = (int)cbParam - 1; i >= 0; i--)
        CPUMPushHyper(pVCpu, paParam[i]);

    STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatWorldSwitch3264, z);
    /* Call the switcher. */
    int rc = pVM->hm.s.pfnHost32ToGuest64R0(pVM, RT_OFFSETOF(VM, aCpus[pVCpu->idCpu].cpum) - RT_OFFSETOF(VM, cpum));
    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatWorldSwitch3264, z);

    /* Restore interrupts. */
    ASMSetFlags(uOldEFlags);
    return rc;
}

#endif /* HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS) */
/** @} */


DECLINLINE(void) hmR0SvmAddXcptIntercept(uint32_t u32Xcpt)
{
    if (!(pVmcb->ctrl.u32InterceptException & RT_BIT(u32Xcpt))
    {
        pVmcb->ctrl.u32InterceptException |= RT_BIT(u32Xcpt);
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
    }
}

DECLINLINE(void) hmR0SvmRemoveXcptIntercept(uint32_t u32Xcpt)
{
#ifndef HMSVM_ALWAYS_TRAP_ALL_XCPTS
    if (pVmcb->ctrl.u32InterceptException & RT_BIT(u32Xcpt))
    {
        pVmcb->ctrl.u32InterceptException &= ~RT_BIT(u32Xcpt);
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
    }
#endif
}


/**
 * Loads the guest control registers (CR0, CR2, CR3, CR4) into the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmLoadGuestControlRegs(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Guest CR0.
     */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_CR0)
    {
        uint64_t u64GuestCR0 = pCtx->cr0;

        /* Always enable caching. */
        u64GuestCR0 &= ~(X86_CR0_CD | X86_CR0_NW);

        /*
         * When Nested Paging is not available use shadow page tables and intercept #PFs (the latter done in SVMR0SetupVM()).
         */
        if (!pVM->hm.s.fNestedPaging)
        {
            u64GuestCR0 |= X86_CR0_PG;  /* When Nested Paging is not available, use shadow page tables. */
            u64GuestCR0 |= X86_CR0_WP;  /* Guest CPL 0 writes to its read-only pages should cause a #PF VM-exit. */
        }

        /*
         * Guest FPU bits.
         */
        bool fInterceptNM = false;
        bool fInterceptMF = false;
        u64GuestCR0 |= X86_CR0_NE;         /* Use internal x87 FPU exceptions handling rather than external interrupts. */
        if (CPUMIsGuestFPUStateActive(pVCpu))
        {
            /* Catch floating point exceptions if we need to report them to the guest in a different way. */
            if (!(u64GuestCR0 & X86_CR0_NE))
            {
                Log4(("hmR0SvmLoadGuestControlRegs: Intercepting Guest CR0.MP Old-style FPU handling!!!\n"));
                pVmcb->ctrl.u32InterceptException |= RT_BIT(X86_XCPT_MF);
                fInterceptMF = true;
            }
        }
        else
        {
            fInterceptNM = true;           /* Guest FPU inactive, VM-exit on #NM for lazy FPU loading. */
            u32GuestCR0 |=  X86_CR0_TS     /* Guest can task switch quickly and do lazy FPU syncing. */
                          | X86_CR0_MP;    /* FWAIT/WAIT should not ignore CR0.TS and should generate #NM. */
        }

        /*
         * Update the exception intercept bitmap.
         */
        if (fInterceptNM)
            hmR0SvmAddXcptIntercept(X86_XCPT_NM);
        else
            hmR0SvmRemoveXcptIntercept(X86_XCPT_NM);

        if (fInterceptMF)
            hmR0SvmAddXcptIntercept(X86_XCPT_MF);
        else
            hmR0SvmRemoveXcptIntercept(X86_XCPT_MF);

        pVmcb->guest.u64CR0 = u64GuestCR0;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX;
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_CR0;
    }

    /*
     * Guest CR2.
     */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_CR2)
    {
        pVmcb->guest.u64CR2 = pCtx->cr2;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CR2;
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_CR2;
    }

    /*
     * Guest CR3.
     */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_CR3)
    {
        if (pVM->hm.s.fNestedPaging)
        {
            PGMMODE enmShwPagingMode;
#if HC_ARCH_BITS == 32
            if (CPUMIsGuestInLongModeEx(pCtx))
                enmShwPagingMode = PGMMODE_AMD64_NX;
            else
#endif
                enmShwPagingMode = PGMGetHostMode(pVM);

            pVmcb->ctrl.u64NestedPagingCR3  = PGMGetNestedCR3(pVCpu, enmShwPagingMode);
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_NP;
            Assert(pVmcb->ctrl.u64NestedPagingCR3);
            pVmcb->guest.u64CR3 = pCtx->cr3;
        }
        else
            pVmcb->guest.u64CR3 = PGMGetHyperCR3(pVCpu);

        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX;
        pVCpu->hm.s.fContextUseFlags &= HM_CHANGED_GUEST_CR3;
    }

    /*
     * Guest CR4.
     */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_CR4)
    {
        uint64_t u64GuestCR4 = pCtx->cr4;
        if (!pVM->hm.s.fNestedPaging)
        {
            switch (pVCpu->hm.s.enmShadowMode)
            {
                case PGMMODE_REAL:
                case PGMMODE_PROTECTED:     /* Protected mode, no paging. */
                    AssertFailed();
                    return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;

                case PGMMODE_32_BIT:        /* 32-bit paging. */
                    u64GuestCR4 &= ~X86_CR4_PAE;
                    break;

                case PGMMODE_PAE:           /* PAE paging. */
                case PGMMODE_PAE_NX:        /* PAE paging with NX enabled. */
                    /** Must use PAE paging as we could use physical memory > 4 GB */
                    u64GuestCR4 |= X86_CR4_PAE;
                    break;

                case PGMMODE_AMD64:         /* 64-bit AMD paging (long mode). */
                case PGMMODE_AMD64_NX:      /* 64-bit AMD paging (long mode) with NX enabled. */
#ifdef VBOX_ENABLE_64_BITS_GUESTS
                    break;
#else
                    AssertFailed();
                    return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;
#endif

                default:                    /* shut up gcc */
                    AssertFailed();
                    return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;
            }
        }

        pVmcb->guest.u64CR4 = u64GuestCR4;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX;
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_CR4;
    }

    return VINF_SUCCESS;
}

/**
 * Loads the guest segment registers into the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadGuestSegmentRegs(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /* Guest Segment registers: CS, SS, DS, ES, FS, GS. */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_SEGMENT_REGS)
    {
        HMSVM_LOAD_SEG_REG(CS, cs);
        HMSVM_LOAD_SEG_REG(SS, cs);
        HMSVM_LOAD_SEG_REG(DS, cs);
        HMSVM_LOAD_SEG_REG(ES, cs);
        HMSVM_LOAD_SEG_REG(FS, cs);
        HMSVM_LOAD_SEG_REG(GS, cs);

        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_SEG;
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_SEGMENT_REGS;
    }

    /* Guest TR. */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_TR)
    {
        HMSVM_LOAD_SEG_REG(TR, tr);
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_TR;
    }

    /* Guest LDTR. */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_LDTR)
    {
        HMSVM_LOAD_SEG_REG(LDTR, ldtr);
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_LDTR;
    }

    /* Guest GDTR. */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_GDTR)
    {
        pVmcb->guest.GDTR.u32Limit = pCtx->gdtr.cbGdt;
        pVmcb->guest.GDTR.u64Base  = pCtx->gdtr.pGdt;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DT;
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_GDTR;
    }

    /* Guest IDTR. */
    if (pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_IDTR)
    {
        pVmcb->guest.IDTR.u32Limit = pCtx->idtr.cbIdt;
        pVmcb->guest.IDTR.u64Base  = pCtx->idtr.pIdt;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DT;
        pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_IDTR;
    }
}


/**
 * Loads the guest MSRs into the VMCB.
 *
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadGuestMsrs(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /* Guest Sysenter MSRs. */
    pVmcb->guest.u64SysEnterCS  = pCtx->SysEnter.cs;
    pVmcb->guest.u64SysEnterEIP = pCtx->SysEnter.eip;
    pVmcb->guest.u64SysEnterESP = pCtx->SysEnter.esp;

    /*
     * Guest EFER MSR.
     * AMD-V requires guest EFER.SVME to be set. Weird.                                                                                 .
     * See AMD spec. 15.5.1 "Basic Operation" | "Canonicalization and Consistency Checks".
     */
    pVmcb->guest.u64EFER = pCtx->msrEFER | MSR_K6_EFER_SVME;

    /* 64-bit MSRs. */
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
        pVmcb->guest.FS.u64Base = pCtx->fs.u64Base;
        pVmcb->guest.GS.u64Base = pCtx->gs.u64Base;
    }
    else
    {
        /* If the guest isn't in 64-bit mode, clear MSR_K6_LME bit from guest EFER otherwise AMD-V expects amd64 shadow paging. */
        pVmcb->guest.u64EFER &= ~MSR_K6_EFER_LME;
    }

    /** @todo The following are used in 64-bit only (SYSCALL/SYSRET) but they might
     *        be writable in 32-bit mode. Clarify with AMD spec. */
    pVmcb->guest.u64STAR         = pCtx->msrSTAR;
    pVmcb->guest.u64LSTAR        = pCtx->msrLSTAR;
    pVmcb->guest.u64CSTAR        = pCtx->msrCSTAR;
    pVmcb->guest.u64SFMASK       = pCtx->msrSFMASK;
    pVmcb->guest.u64KernelGSBase = pCtx->msrKERNELGSBASE;
}


/**
 * Loads the guest debug registers into the VMCB.
 *
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadGuestDebugRegs(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    if (!(pVCpu->hm.s.fContextUseFlags & HM_CHANGED_GUEST_DEBUG))
        return;

    /** @todo Turn these into assertions if possible. */
    pCtx->dr[6] |= X86_DR6_INIT_VAL;                                          /* Set reserved bits to 1. */
    pCtx->dr[6] &= ~RT_BIT(12);                                               /* MBZ. */

    pCtx->dr[7] &= 0xffffffff;                                                /* Upper 32 bits MBZ. */
    pCtx->dr[7] &= ~(RT_BIT(11) | RT_BIT(12) | RT_BIT(14) | RT_BIT(15));      /* MBZ. */
    pCtx->dr[7] |= 0x400;                                                     /* MB1. */

    /* Update DR6, DR7 with the guest values. */
    pVmcb->guest.u64DR7 = pCtx->dr[7];
    pVmcb->guest.u64DR6 = pCtx->dr[6];
    pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DRX;

    bool fInterceptDB     = false;
    bool fInterceptMovDRx = false;
    if (DBGFIsStepping(pVCpu))
    {
        /* AMD-V doesn't have any monitor-trap flag equivalent. Instead, enable tracing in the guest and trap #DB. */
        pVmcb->guest.u64RFlags |= X86_EFL_TF;
        fInterceptDB = true;
    }

    if (CPUMGetHyperDR7(pVCpu) & (X86_DR7_ENABLED_MASK | X86_DR7_GD))
    {
        if (!CPUMIsHyperDebugStateActive(pVCpu))
        {
            rc = CPUMR0LoadHyperDebugState(pVM, pVCpu, pCtx, true /* include DR6 */);
            AssertRC(rc);

            /* Update DR6, DR7 with the hypervisor values. */
            pVmcb->guest.u64DR7 = CPUMGetHyperDR7(pVCpu);
            pVmcb->guest.u64DR6 = CPUMGetHyperDR6(pVCpu);
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DRX;
        }
        Assert(CPUMIsHyperDebugStateActive(pVCpu));
        fInterceptMovDRx = true;
    }
    else if (pCtx->dr[7] & (X86_DR7_ENABLED_MASK | X86_DR7_GD))
    {
        if (!CPUMIsGuestDebugStateActive(pVCpu))
        {
            rc = CPUMR0LoadGuestDebugState(pVM, pVCpu, pCtx, true /* include DR6 */);
            AssertRC(rc);
            STAM_COUNTER_INC(&pVCpu->hm.s.StatDRxArmed);
        }
        Assert(CPUMIsGuestDebugStateActive(pVCpu));
        Assert(fInterceptMovDRx == false);
    }
    else if (!CPUMIsGuestDebugStateActive(pVCpu))
    {
        /* For the first time we would need to intercept MOV DRx accesses even when the guest debug registers aren't loaded. */
        fInterceptMovDRx = true;
    }

    if (fInterceptDB)
        hmR0SvmAddXcptIntercept(X86_XCPT_DB);
    else
        hmR0SvmRemoveXcptIntercept(X86_XCPT_DB);

    if (fInterceptMovDRx)
    {
        if (   pVmcb->ctrl.u16InterceptRdDRx != 0xffff
            || pVmcb->ctrl.u16InterceptWrDRx != 0xffff)
        {
            pVmcb->ctrl.u16InterceptRdDRx = 0xffff;
            pVmcb->ctrl.u16InterceptWrDRx = 0xffff;
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
        }
    }
    else
    {
        if (   pVmcb->ctrl.u16InterceptRdDRx
            || pVmcb->ctrl.u16InterceptWrDRx)
        {
            pVmcb->ctrl.u16InterceptRdDRx = 0;
            pVmcb->ctrl.u16InterceptWrDRx = 0;
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
        }
    }

    pVCpu->hm.s.fContextUseFlags &= ~HM_CHANGED_GUEST_DEBUG;
}

/**
 * Sets up the appropriate function to run guest code.
 *
 * @returns VBox status code.
 * @param   pVCpu   Pointer to the VMCPU.
 * @param   pCtx    Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmSetupVMRunHandler(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
#ifndef VBOX_ENABLE_64_BITS_GUESTS
        return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;
#endif
        Assert(pVCpu->CTX_SUFF(pVM)->hm.s.fAllow64BitGuests);    /* Guaranteed by hmR3InitFinalizeR0(). */
#if HC_ARCH_BITS == 32 && !defined(VBOX_WITH_HYBRID_32BIT_KERNEL)
        /* 32-bit host. We need to switch to 64-bit before running the 64-bit guest. */
        pVCpu->hm.s.svm.pfnVMRun = SVMR0VMSwitcherRun64;
#else
        /* 64-bit host or hybrid host. */
        pVCpu->hm.s.svm.pfnVMRun = SVMR0VMRun64;
#endif
    }
    else
    {
        /* Guest is not in long mode, use the 32-bit handler. */
        pVCpu->hm.s.svm.pfnVMRun = SVMR0VMRun;
    }
    return VINF_SUCCESS;
}


/**
 * Enters the AMD-V session.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCpu        Pointer to the CPU info struct.
 */
VMMR0DECL(int) SVMR0Enter(PVM pVM, PVMCPU pVCpu, PHMGLOBLCPUINFO pCpu)
{
    AssertPtr(pVM);
    AssertPtr(pVCpu);
    Assert(pVM->hm.s.svm.fSupported);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    NOREF(pCpu);

    LogFlowFunc(("pVM=%p pVCpu=%p\n", pVM, pVCpu));

    /* Nothing to do here. */
    return VINF_SUCCESS;
}


/**
 * Leaves the AMD-V session.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
VMMR0DECL(int) SVMR0Leave(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    NOREF(pVM);
    NOREF(pVCpu);
    NOREF(pCtx);

    /* Nothing to do here. Everything is taken care of in hmR0SvmLongJmpToRing3(). */
    return VINF_SUCCESS;
}


/**
 * Saves the host state.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 *
 * @remarks No-long-jump zone!!!
 */
VMMR0DECL(int) SVMR0SaveHostState(PVM pVM, PVMCPU pVCpu)
{
    NOREF(pVM);
    NOREF(pVCpu);
    /* Nothing to do here. AMD-V does this for us automatically during the world-switch. */
    return VINF_SUCCESS;
}


/**
 * Loads the guest state.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
VMMR0DECL(int) SVMR0LoadGuestState(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    AssertPtr(pVM);
    AssertPtr(pVCpu);
    AssertPtr(pCtx);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    PSVMVMCB pVmcb = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;
    AssertMsgReturn(pVmcb, ("Invalid pVmcb\n"), VERR_SVM_INVALID_PVMCB);

    STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatLoadGuestState, x);

    int rc = hmR0SvmLoadGuestControlRegs(pVCpu, pCtx);
    AssertLogRelMsgRCReturn(rc, ("hmR0SvmLoadGuestControlRegs! rc=%Rrc (pVM=%p pVCpu=%p)\n", rc, pVM, pVCpu), rc);

    hmR0SvmLoadGuestSegmentRegs(pVCpu, pCtx);
    hmR0SvmLoadGuestMsrs(pVCpu, pCtx);

    pVmcb->guest.u64RIP    = pCtx->rip;
    pVmcb->guest.u64RSP    = pCtx->rsp;
    pVmcb->guest.u64RFlags = pCtx->eflags.u32;
    pVmcb->guest.u8CPL     = pCtx->ss.Attr.n.u2Dpl;
    pVmcb->guest.u64RAX    = pCtx->rax;

    /* hmR0SvmLoadGuestDebugRegs() must be called -after- updating guest RFLAGS as the RFLAGS may need to be changed. */
    hmR0SvmLoadGuestDebugRegs(pVCpu, pCtx);

    rc = hmR0SvmSetupVMRunHandler(pVCpu, pCtx);
    AssertLogRelMsgRCReturn(rc, ("hmR0SvmSetupVMRunHandler! rc=%Rrc (pVM=%p pVCpu=%p)\n", rc, pVM, pVCpu), rc);

    /* Clear any unused and reserved bits. */
    pVCpu->hm.s.fContextUseFlags &= ~(  HM_CHANGED_GUEST_SYSENTER_CS_MSR
                                      | HM_CHANGED_GUEST_SYSENTER_EIP_MSR
                                      | HM_CHANGED_GUEST_SYSENTER_ESP_MSR);

    AssertMsg(!pVCpu->hm.s.fContextUseFlags,
             ("Missed updating flags while loading guest state. pVM=%p pVCpu=%p fContextUseFlags=%#RX32\n",
              pVM, pVCpu, pVCpu->hm.s.fContextUseFlags));

    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatLoadGuestState, x);

    return rc;
}


/**
 * Saves the entire guest state from the VMCB into the
 * guest-CPU context. Currently there is no residual state left in the CPU that
 * is not updated in the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   pMixedCtx       Pointer to the guest-CPU context. The data may be
 *                          out-of-sync. Make sure to update the required fields
 *                          before using them.
 */
static void hmR0SvmSaveGuestState(PVMCPU pVCpu, PCPUMCTX pMixedCtx)
{
    Assert(VMMRZCallRing3IsEnabled(pVCpu));

    PSVMVMCB pVmcb        = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;

    pMixedCtx->rip        = pVmcb->guest.u64RIP;
    pMixedCtx->rsp        = pVmcb->guest.u64RSP;
    pMixedCtx->eflags.u32 = pVmcb->guest.u64RFlags;
    pMixedCtx->rax        = pVmcb->guest.u64RAX;

    /*
     * Control registers: CR2, CR3 (handled at the end) - accesses to other control registers are always intercepted.
     */
    pMixedCtx->cr2        = pVmcb->guest.u64CR2;

    /*
     * Guest MSRs.
     */
    pMixedCtx->msrSTAR         = pVmcb->guest.u64STAR;            /* legacy syscall eip, cs & ss */
    pMixedCtx->msrLSTAR        = pVmcb->guest.u64LSTAR;           /* 64-bit mode syscall rip */
    pMixedCtx->msrCSTAR        = pVmcb->guest.u64CSTAR;           /* compatibility mode syscall rip */
    pMixedCtx->msrSFMASK       = pVmcb->guest.u64SFMASK;          /* syscall flag mask */
    pMixedCtx->msrKERNELGSBASE = pVmcb->guest.u64KernelGSBase;    /* swapgs exchange value */
    pMixedCtx->SysEnter.cs     = pVmcb->guest.u64SysEnterCS;
    pMixedCtx->SysEnter.eip    = pVmcb->guest.u64SysEnterEIP;
    pMixedCtx->SysEnter.esp    = pVmcb->guest.u64SysEnterESP;

    /*
     * Guest segment registers (includes FS, GS base MSRs for 64-bit guests).
     */
    HMSVM_SAVE_SEG_REG(CS, ss);
    HMSVM_SAVE_SEG_REG(SS, cs);
    HMSVM_SAVE_SEG_REG(DS, ds);
    HMSVM_SAVE_SEG_REG(ES, es);
    HMSVM_SAVE_SEG_REG(FS, fs);
    HMSVM_SAVE_SEG_REG(GS, gs);

    /*
     * Correct the hidden CS granularity flag. Haven't seen it being wrong in any other
     * register (yet).
     */
    /** @todo Verify this. */
    if (   !pMixedCtx->cs.Attr.n.u1Granularity
        &&  pMixedCtx->cs.Attr.n.u1Present
        &&  pMixedCtx->cs.u32Limit > UINT32_C(0xfffff))
    {
        Assert((pMixedCtx->cs.u32Limit & 0xfff) == 0xfff);
        pMixedCtx->cs.Attr.n.u1Granularity = 1;
    }
#ifdef VBOX_STRICT
# define HMSVM_ASSERT_SEL_GRANULARITY(reg) \
    AssertMsg(   !pMixedCtx->reg.Attr.n.u1Present \
              || (   pMixedCtx->reg.Attr.n.u1Granularity \
                  ? (pMixedCtx->reg.u32Limit & 0xfff) == 0xfff \
                  :  pMixedCtx->reg.u32Limit <= UINT32_C(0xfffff)), \
              ("Invalid Segment Attributes %#x %#x %#llx\n", pMixedCtx->reg.u32Limit,
              pMixedCtx->reg.Attr.u, pMixedCtx->reg.u64Base))

    HMSVM_ASSERT_SEG_GRANULARITY(cs);
    HMSVM_ASSERT_SEG_GRANULARITY(ss);
    HMSVM_ASSERT_SEG_GRANULARITY(ds);
    HMSVM_ASSERT_SEG_GRANULARITY(es);
    HMSVM_ASSERT_SEG_GRANULARITY(fs);
    HMSVM_ASSERT_SEG_GRANULARITY(gs);

# undef HMSVM_ASSERT_SEL_GRANULARITY
#endif

    /*
     * Sync the hidden SS DPL field. AMD CPUs have a separate CPL field in the VMCB and uses that
     * and thus it's possible that when the CPL changes during guest execution that the SS DPL
     * isn't updated by AMD-V. Observed on some AMD Fusion CPUs with 64-bit guests.
     * See AMD spec. 15.5.1 "Basic operation".
     */
    Assert(!(pVmcb->guest.u8CPL & ~0x3));
    pMixedCtx->ss.Attr.n.u2Dpl = pVmcb->guest.u8CPL & 0x3;

    /*
     * Guest Descriptor-Table registers.
     */
    HMSVM_SAVE_SEG_REG(TR, tr);
    HMSVM_SAVE_SEG_REG(LDTR, ldtr);
    pMixedCtx->gdtr.cbGdt = pVmcb->guest.GDTR.u32Limit;
    pMixedCtx->gdtr.pGdt  = pVmcb->guest.GDTR.u64Base;

    pMixedCtx->idtr.cbIdt = pVmcb->guest.IDTR.u32Limit;
    pMixedCtx->idtr.pIdt  = pVmcb->guest.IDTR.u64Base;

    /*
     * Guest Debug registers.
     */
    pMixedCtx->dr[6] = pVmcb->guest.u64DR6;
    pMixedCtx->dr[7] = pVmcb->guest.u64DR7;

    /*
     * With Nested Paging, CR3 changes are not intercepted. Therefore, sync. it now.
     * This is done as the very last step of syncing the guest state, as PGMUpdateCR3() may cause longjmp's to ring-3.
     */
    if (   pVM->hm.s.fNestedPaging
        && pMixedCtx->cr3 != pVmcb->guest.u64CR3)
    {
        CPUMSetGuestCR3(pVCpu, pVmcb->guest.u64CR3);
        PGMUpdateCR3(pVCpu, pVmcb->guest.u64CR3);
    }
}


/**
 * Does the necessary state syncing before doing a longjmp to ring-3.
 *
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   rcExit      The reason for exiting to ring-3. Can be
 *                      VINF_VMM_UNKNOWN_RING3_CALL.
 *
 * @remarks No-long-jmp zone!!!
 */
static void hmR0SvmLongJmpToRing3(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rcExit)
{
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));
    Assert(VMMR0IsLogFlushDisabled(pVCpu));

    /* Restore host FPU state if necessary and resync on next R0 reentry .*/
    if (CPUMIsGuestFPUStateActive(pVCpu))
    {
        CPUMR0SaveGuestFPU(pVM, pVCpu, pCtx);
        Assert(!CPUMIsGuestFPUStateActive(pVCpu));
        pVCpu->hm.s.fContextUseFlags |= HM_CHANGED_GUEST_CR0;
    }

    /* Restore host debug registers if necessary and resync on next R0 reentry. */
    if (CPUMIsGuestDebugStateActive(pVCpu))
    {
        CPUMR0SaveGuestDebugState(pVM, pVCpu, pCtx, true /* save DR6 */);
        Assert(!CPUMIsGuestDebugStateActive(pVCpu));
        pVCpu->hm.s.fContextUseFlags |= HM_CHANGED_GUEST_DEBUG;
    }
    else if (CPUMIsHyperDebugStateActive(pVCpu))
    {
        CPUMR0LoadHostDebugState(pVM, pVCpu);
        Assert(!CPUMIsHyperDebugStateActive(pVCpu));
        Assert(pVmcb->ctrl.u16InterceptRdDRx == 0xffff);
        Assert(pVmcb->ctrl.u16InterceptWrDRx == 0xffff);
    }

    STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchLongJmpToR3);
    VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_HM, VMCPUSTATE_STARTED_EXEC);
}


/**
 * VMMRZCallRing3() callback wrapper which saves the guest state (or restores
 * any remaining host state) before we longjump to ring-3 and possibly get
 * preempted.
 *
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   enmOperation    The operation causing the ring-3 longjump.
 * @param   pvUser          The user argument (pointer to the possibly
 *                          out-of-date guest-CPU context).
 *
 * @remarks Must never be called with @a enmOperation ==
 *          VMMCALLRING3_VM_R0_ASSERTION.
 */
DECLCALLBACK(void) hmR0SvmCallRing3Callback(PVMCPU pVCpu, VMMCALLRING3 enmOperation, void *pvUser)
{
    /* VMMRZCallRing3() already makes sure we never get called as a result of an longjmp due to an assertion, */
    Assert(pVCpu);
    Assert(pvUser);
    Assert(VMMRZCallRing3IsEnabled(pVCpu));
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    VMMRZCallRing3Disable(pVCpu);
    Assert(VMMR0IsLogFlushDisabled(pVCpu));
    Log4(("hmR0SvmCallRing3Callback->hmR0SvmLongJmpToRing3\n"));
    hmR0SvmLongJmpToRing3(pVCpu->CTX_SUFF(pVM), pVCpu, (PCPUMCTX)pvUser, VINF_VMM_UNKNOWN_RING3_CALL);
    VMMRZCallRing3Enable(pVCpu);
}


/**
 * An action requires us to go back to ring-3. This function does the necessary
 * steps before we can safely return to ring-3. This is not the same as longjmps
 * to ring-3, this is voluntary.
 *
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   rcExit      The reason for exiting to ring-3. Can be
 *                      VINF_VMM_UNKNOWN_RING3_CALL.
 */
static void hmR0SvmExitToRing3(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rcExit)
{
    Assert(pVM);
    Assert(pVCpu);
    Assert(pCtx);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    if (RT_UNLIKELY(rcExit == VERR_SVM_INVALID_GUEST_STATE))
    {
        /* We don't need to do any syncing here, we're not going to come back to execute anything again. */
        return;
    }

    /* Please, no longjumps here (any logging shouldn't flush jump back to ring-3). NO LOGGING BEFORE THIS POINT! */
    VMMRZCallRing3Disable(pVCpu);
    Log4(("hmR0SvmExitToRing3: rcExit=%d\n", rcExit));

    /* We need to do this only while truly exiting the "inner loop" back to ring-3 and -not- for any longjmp to ring3. */
    if (pVCpu->hm.s.Event.fPending)
    {
        hmR0SvmPendingEventToTrpmTrap(pVCpu);
        Assert(!pVCpu->hm.s.Event.fPending);
    }

    /* Sync. the guest state. */
    hmR0SvmLongJmpToRing3(pVM, pVCpu, pCtx, rcExit);
    STAM_COUNTER_DEC(&pVCpu->hm.s.StatSwitchLongJmpToR3);

    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TO_R3);
    CPUMSetChangedFlags(pVCpu,  CPUM_CHANGED_SYSENTER_MSR
                              | CPUM_CHANGED_LDTR
                              | CPUM_CHANGED_GDTR
                              | CPUM_CHANGED_IDTR
                              | CPUM_CHANGED_TR
                              | CPUM_CHANGED_HIDDEN_SEL_REGS);

    /* On our way back from ring-3 the following needs to be done. */
    /** @todo This can change with preemption hooks. */
    if (rcExit == VINF_EM_RAW_INTERRUPT)
        pVCpu->hm.s.fContextUseFlags |= HM_CHANGED_HOST_CONTEXT;
    else
        pVCpu->hm.s.fContextUseFlags |= HM_CHANGED_HOST_CONTEXT | HM_CHANGED_ALL_GUEST;

    STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchExitToR3);
    VMMRZCallRing3Enable(pVCpu);
}


/**
 * Sets up the usage of TSC offsetting for the VCPU.
 *
 * @param   pVCpu       Pointer to the VMCPU.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmSetupTscOffsetting(PVMCPU pVCpu)
{
    PSVMVMCB pVmcb = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;
    if (TMCpuTickCanUseRealTSC(pVCpu, &pVmcb->ctrl.u64TSCOffset))
    {
        uint64_t u64CurTSC = ASMReadTSC();
        if (u64CurTSC + pVmcb->ctrl.u64TSCOffset > TMCpuTickGetLastSeen(pVCpu))
        {
            pVmcb->ctrl.u32InterceptCtrl1 &= ~SVM_CTRL1_INTERCEPT_RDTSC;
            pVmcb->ctrl.u32InterceptCtrl2 &= ~SVM_CTRL2_INTERCEPT_RDTSCP;
            STAM_COUNTER_INC(&pVCpu->hm.s.StatTscOffset);
        }
        else
        {
            pVmcb->ctrl.u32InterceptCtrl1 |= SVM_CTRL1_INTERCEPT_RDTSC;
            pVmcb->ctrl.u32InterceptCtrl2 |= SVM_CTRL2_INTERCEPT_RDTSCP;
            STAM_COUNTER_INC(&pVCpu->hm.s.StatTscInterceptOverFlow);
        }
    }
    else
    {
        pVmcb->ctrl.u32InterceptCtrl1 |= SVM_CTRL1_INTERCEPT_RDTSC;
        pVmcb->ctrl.u32InterceptCtrl2 |= SVM_CTRL2_INTERCEPT_RDTSCP;
        STAM_COUNTER_INC(&pVCpu->hm.s.StatTscIntercept);
    }

    pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
}


/**
 * Sets an event as a pending event to be injected into the guest.
 *
 * @param   pVCpu               Pointer to the VMCPU.
 * @param   pEvent              Pointer to the SVM event.
 * @param   GCPtrFaultAddress   The fault-address (CR2) in case it's a
 *                              page-fault.
 */
DECLINLINE(void) hmR0SvmSetPendingEvent(PVMCPU pVCpu, PSVMEVENT pEvent, RTGCUINTPTR GCPtrFaultAddress)
{
    Assert(!pVCpu->hm.s.Event.fPending);

    pVCpu->hm.s.Event.u64IntrInfo       = pEvent->u;
    pVCpu->hm.s.Event.fPending          = true;
    pVCpu->hm.s.Event.GCPtrFaultAddress = GCPtrFaultAddress;

#ifdef VBOX_STRICT
    if (GCPtrFaultAddress)
    {
        AssertMsg(   pEvent->n.u8Vector == X86_XCPT_PF
                  && pEvent->n.u3Type   == SVM_EVENT_EXCEPTION,
                  ("hmR0SvmSetPendingEvent: Setting fault-address for non-#PF. u8Vector=%#x Type=%#RX32 GCPtrFaultAddr=%#RGx\n",
                   pEvent->n.u8Vector, (uint32_t)pEvent->n.u3Type, GCPtrFaultAddress));
        Assert(GCPtrFaultAddress == CPUMGetGuestCR2(pVCpu));
    }
#endif

    Log4(("hmR0SvmSetPendingEvent: u=%#RX64 u8Vector=%#x ErrorCodeValid=%#x ErrorCode=%#RX32\n", pEvent->u,
          pEvent->n.u8Vector, pEvent->n.u3Type, (uint8_t)pEvent->n.u1ErrorCodeValid, pEvent->n.u32ErrorCode));
}


/**
 * Injects an event into the guest upon VMRUN by updating the relevant field
 * in the VMCB.
 *
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pVmcb       Pointer to the guest VMCB.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pEvent      Pointer to the event.
 *
 * @remarks No-long-jump zone!!!
 * @remarks Requires CR0!
 */
DECLINLINE(void) hmR0SvmInjectEventVmcb(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx, PSVMEVENT pEvent)
{
    pVmcb->ctrl.EventInject.u = pEvent->u;
    STAM_COUNTER_INC(&pVCpu->hm.s.paStatInjectedIrqsR0[pEvent->n.u8Vector & MASK_INJECT_IRQ_STAT]);
}


/**
 * Converts any TRPM trap into a pending SVM event. This is typically used when
 * entering from ring-3 (not longjmp returns).
 *
 * @param   pVCpu           Pointer to the VMCPU.
 */
static void hmR0SvmTrpmTrapToPendingEvent(PVMCPU pVCpu)
{
    Assert(TRPMHasTrap(pVCpu));
    Assert(!pVCpu->hm.s.Event.fPending);

    uint8_t     uVector;
    TRPMEVENT   enmTrpmEvent;
    RTGCUINT    uErrCode;
    RTGCUINTPTR GCPtrFaultAddress;
    uint8_t     cbInstr;

    int rc = TRPMQueryTrapAll(pVCpu, &uVector, &enmTrpmEvent, &uErrCode, &GCPtrFaultAddress, &cbInstr);
    AssertRC(rc);

    PSVMEVENT pEvent = &pVCpu->hm.s.Event;
    pEvent->u         = 0;
    pEvent->n.u1Valid = 1;

    /* Refer AMD spec. 15.20 "Event Injection" for the format. */
    if (enmTrpmEvent == TRPM_TRAP)
    {
        pEvent->n.u3Type = SVM_EVENT_EXCEPTION;
        switch (uVector)
        {
            case X86_XCPT_PF:
            case X86_XCPT_DF:
            case X86_XCPT_TS:
            case X86_XCPT_NP:
            case X86_XCPT_SS:
            case X86_XCPT_GP:
            case X86_XCPT_AC:
            {
                pEvent->n.u32ErrorCode     = uErrCode;
                pEvent->n.u1ErrorCodeValid = 1;
                break;
            }
        }
    }
    else if (enmTrpmEvent == TRPM_HARDWARE_INT)
    {
        if (uVector == X86_XCPT_NMI)
            pEvent->n.u3Type = SVM_EVENT_NMI;
        else
            pEvent->n.u3Type = SVM_EVENT_EXTERNAL_IRQ;
    }
    else if (enmTrpmEvent == TRPM_SOFTWARE_INT)
        pEvent->n.u3Type = SVM_EVENT_SOFTWARE_INT;
    else
        AssertMsgFailed(("Invalid TRPM event type %d\n", enmTrpmEvent));

    rc = TRPMResetTrap(pVCpu);
    AssertRC(rc);

    Log4(("TRPM->HM event: u=%#RX64 u8Vector=%#x uErrorCodeValid=%#x uErrorCode=%#RX32\n", pEvent->u, pEvent->n.u8Vector,
          pEvent->n.u1ErrorCodeValid, pEvent->n.u32ErrorCode));
}


/**
 * Converts any pending SVM event into a TRPM trap. Typically used when leaving
 * AMD-V to execute any instruction.
 *
 * @param   pvCpu           Pointer to the VMCPU.
 */
static void hmR0SvmPendingEventToTrpmTrap(PVMCPU pVCpu)
{
    Assert(pVCpu->hm.s.Event.fPending);
    Assert(TRPMQueryTrap(pVCpu, NULL /* pu8TrapNo */, NULL /* pEnmType */) == VERR_TRPM_NO_ACTIVE_TRAP);

    PSVMEVENT pEvent    = &pVCpu->hm.s.Event;
    uint8_t uVector     = pEvent->n.u8Vector;
    uint8_t uVectorType = pEvent->n.u3Type;

    TRPMEVENT enmTrapType;
    switch (uVectorType)
    {
        case SVM_EVENT_EXTERNAL_IRQ
        case SVM_EVENT_NMI:
           enmTrapType = TRPM_HARDWARE_INT;
           break;
        case SVM_EVENT_SOFTWARE_INT:
            enmTrapType = TRPM_SOFTWARE_INT;
            break;
        case SVM_EVENT_EXCEPTION:
            enmTrapType = TRPM_TRAP;
            break;
        default:
            AssertMsgFailed(("Invalid pending-event type %#x\n", uVectorType));
            enmTrapType = TRPM_32BIT_HACK;
            break;
    }

    Log4(("HM event->TRPM: uVector=%#x enmTrapType=%d\n", uVector, uVectorType));

    int rc = TRPMAssertTrap(pVCpu, uVector, enmTrapType);
    AssertRC(rc);

    if (pEvent->n.u1ErrorCodeValid)
        TRPMSetErrorCode(pVCpu, pEvent->n.u32ErrorCode);

    if (   uVectorType == SVM_EVENT_EXCEPTION
        && uVector     == X86_XCPT_PF)
    {
        TRPMSetFaultAddress(pVCpu, pVCpu->hm.s.Event.GCPtrFaultAddress);
        Assert(pVCpu->hm.s.Event.GCPtrFaultAddress == CPUMGetGuestCR2(pVCpu));
    }
    else if (uVectorType == SVM_EVENT_SOFTWARE_INT)
    {
        AssertMsg(   uVectorType == SVM_EVENT_SOFTWARE_INT
                  || (uVector == X86_XCPT_BP || uVector == X86_XCPT_OF),
                  ("Invalid vector: uVector=%#x uVectorType=%#x\n", uVector, uVectorType));
        TRPMSetInstrLength(pVCpu, pVCpu->hm.s.Event.cbInstr);
    }
    pVCpu->hm.s.Event.fPending = false;
}


/**
 * Gets the guest's interrupt-shadow.
 *
 * @returns The guest's interrupt-shadow.
 * @param   pVCpu   Pointer to the VMCPU.
 * @param   pCtx    Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 * @remarks Has side-effects with VMCPU_FF_INHIBIT_INTERRUPTS force-flag.
 */
DECLINLINE(uint32_t) hmR0SvmGetGuestIntrShadow(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Instructions like STI and MOV SS inhibit interrupts till the next instruction completes. Check if we should
     * inhibit interrupts or clear any existing interrupt-inhibition.
     */
    uint32_t uIntrState = 0;
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    {
        if (pCtx->rip != EMGetInhibitInterruptsPC(pVCpu))
        {
            /*
             * We can clear the inhibit force flag as even if we go back to the recompiler without executing guest code in
             * AMD-V, the flag's condition to be cleared is met and thus the cleared state is correct.
             */
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
        }
        else
            uIntrState = SVM_INTERRUPT_SHADOW_ACTIVE;
    }
    return uIntrState;
}


/**
 * Sets the virtual interrupt intercept control in the VMCB which
 * instructs AMD-V to cause a #VMEXIT as soon as the guest is in a state to
 * receive interrupts.
 *
 * @param pVmcb         Pointer to the VMCB.
 */
DECLINLINE(void) hmR0SvmSetVirtIntrIntercept(PSVMVMCB pVmcb)
{
    if (!(pVmcb->ctrl.u32InterceptCtrl1 & SVM_CTRL1_INTERCEPT_VINTR))
    {
        pVmcb->ctrl.IntCtrl.n.u1VIrqValid  = 1;     /* A virtual interrupt is pending. */
        pVmcb->ctrl.IntCtrl.n.u8VIrqVector = 0;     /* Not necessary as we #VMEXIT for delivering the interrupt. */
        pVmcb->ctrl.u32InterceptCtrl1 |= SVM_CTRL1_INTERCEPT_VINTR;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
    }
}


/**
 * Injects any pending events into the guest if the guest is in a state to
 * receive them.
 *
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
static void hmR0SvmInjectPendingEvent(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(!TRPMHasTrap(pVCpu));

    const bool fIntShadow = !!hmR0SvmGetGuestIntrShadow(pVCpu, pCtx);
    PSVMVMCB pVmcb        = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;

    SVMEVENT Event;
    Event.u = 0;
    if (pVCpu->hm.s.Event.fPending)                            /* First, inject any pending HM events. */
    {
        Event.u = pVCpu->hm.s.Event.u64IntrInfo;
        bool fInject = true;
        if (   fIntShadow
            && (   Event.n.u3Type == SVM_EVENT_EXTERNAL_IRQ
                || Event.n.u3Type == SVM_EVENT_NMI))
        {
            fInject = false;
        }

        if (   fInject
            && Event.n.u1Valid)
        {
            pVCpu->hm.s.Event.fPending = false;
            hmR0SvmInjectEvent(pVCpu, pVmcb, pCtx, &Event);
        }
        else
            hmR0SvmSetVirtIntrIntercept(pVmcb);
    }                                                          /** @todo SMI. SMIs take priority over NMIs. */
    else if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_NMI))   /* NMI. NMIs take priority over regular interrupts . */
    {
        if (!fIntShadow)
        {
            Log4(("Injecting NMI\n"));
            Event.n.u1Valid      = 1;
            Event.n.u8Vector     = X86_XCPT_NMI;
            Event.n.u3Type       = SVM_EVENT_NMI;

            hmR0SvmInjectEvent(pVCpu, pVmcb, pCtx, &Event);
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_NMI);
        }
        else
            hmR0SvmSetVirtIntrIntercept(pVmcb);
    }
    else if (VMCPU_FF_IS_PENDING(pVCpu, (VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)))
    {
        /* Check if there are guest external interrupts (PIC/APIC) pending and inject them if the guest can receive them. */
        const bool fBlockInt = !(pCtx->eflags.u32 & X86_EFL_IF);
        if (   !fBlockInt
            && !fIntShadow)
        {
            uint8_t u8Interrupt;
            rc = PDMGetInterrupt(pVCpu, &u8Interrupt);
            if (RT_SUCCESS(rc))
            {
                Log4(("Injecting interrupt u8Interrupt=%#x\n", u8Interrupt));

                Event.n.u1Valid  = 1;
                Event.n.u8Vector = u8Interrupt;
                Event.n.u3Type   = SVM_EVENT_EXTERNAL_IRQ;

                hmR0SvmInjectEvent(pVCpu, pVmcb, pCtx, &Event);
                STAM_COUNTER_INC(&pVCpu->hm.s.StatIntInject);
            }
            else
            {
                /** @todo Does this actually happen? If not turn it into an assertion. */
                Assert(!VMCPU_FF_IS_PENDING(pVCpu, (VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)));
                STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchGuestIrq);
            }
        }
        else
            hmR0SvmSetVirtIntrIntercept(pVmcb);
    }

    /* Update the guest interrupt shadow in the VMCB. */
    pVmcb->ctrl.u64IntShadow = !!fIntShadow;
}


/**
 * Reports world-switch error and dumps some useful debug info.
 *
 * @param   pVM             Pointer to the VM.
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   rcVMRun         The return code from VMRUN (or
 *                          VERR_SVM_INVALID_GUEST_STATE for invalid
 *                          guest-state).
 * @param   pCtx            Pointer to the guest-CPU context.
 */
static void hmR0SvmReportWorldSwitchError(PVM pVM, PVMCPU pVCpu, int rcVMRun, PCPUMCTX pCtx)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    PSVMVMCB pVmcb = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;

    if (rcVMRun == VERR_SVM_INVALID_GUEST_STATE)
    {
        HMDumpRegs(pVM, pVCpu, pCtx);
#ifdef VBOX_STRICT
        Log4(("ctrl.u64VmcbCleanBits             %#RX64\n",   pVmcb->ctrl.u64VmcbCleanBits));
        Log4(("ctrl.u16InterceptRdCRx            %#x\n",      pVmcb->ctrl.u16InterceptRdCRx));
        Log4(("ctrl.u16InterceptWrCRx            %#x\n",      pVmcb->ctrl.u16InterceptWrCRx));
        Log4(("ctrl.u16InterceptRdDRx            %#x\n",      pVmcb->ctrl.u16InterceptRdDRx));
        Log4(("ctrl.u16InterceptWrDRx            %#x\n",      pVmcb->ctrl.u16InterceptWrDRx));
        Log4(("ctrl.u32InterceptException        %#x\n",      pVmcb->ctrl.u32InterceptException));
        Log4(("ctrl.u32InterceptCtrl1            %#x\n",      pVmcb->ctrl.u32InterceptCtrl1));
        Log4(("ctrl.u32InterceptCtrl2            %#x\n",      pVmcb->ctrl.u32InterceptCtrl2));
        Log4(("ctrl.u64IOPMPhysAddr              %#RX64\n",   pVmcb->ctrl.u64IOPMPhysAddr));
        Log4(("ctrl.u64MSRPMPhysAddr             %#RX64\n",   pVmcb->ctrl.u64MSRPMPhysAddr));
        Log4(("ctrl.u64TSCOffset                 %#RX64\n",   pVmcb->ctrl.u64TSCOffset));

        Log4(("ctrl.TLBCtrl.u32ASID              %#x\n",      pVmcb->ctrl.TLBCtrl.n.u32ASID));
        Log4(("ctrl.TLBCtrl.u8TLBFlush           %#x\n",      pVmcb->ctrl.TLBCtrl.n.u8TLBFlush));
        Log4(("ctrl.TLBCtrl.u24Reserved          %#x\n",      pVmcb->ctrl.TLBCtrl.n.u24Reserved));

        Log4(("ctrl.IntCtrl.u8VTPR               %#x\n",      pVmcb->ctrl.IntCtrl.n.u8VTPR));
        Log4(("ctrl.IntCtrl.u1VIrqValid          %#x\n",      pVmcb->ctrl.IntCtrl.n.u1VIrqValid));
        Log4(("ctrl.IntCtrl.u7Reserved           %#x\n",      pVmcb->ctrl.IntCtrl.n.u7Reserved));
        Log4(("ctrl.IntCtrl.u4VIrqPriority       %#x\n",      pVmcb->ctrl.IntCtrl.n.u4VIrqPriority));
        Log4(("ctrl.IntCtrl.u1IgnoreTPR          %#x\n",      pVmcb->ctrl.IntCtrl.n.u1IgnoreTPR));
        Log4(("ctrl.IntCtrl.u3Reserved           %#x\n",      pVmcb->ctrl.IntCtrl.n.u3Reserved));
        Log4(("ctrl.IntCtrl.u1VIrqMasking        %#x\n",      pVmcb->ctrl.IntCtrl.n.u1VIrqMasking));
        Log4(("ctrl.IntCtrl.u6Reserved           %#x\n",      pVmcb->ctrl.IntCtrl.n.u6Reserved));
        Log4(("ctrl.IntCtrl.u8VIrqVector         %#x\n",      pVmcb->ctrl.IntCtrl.n.u8VIrqVector));
        Log4(("ctrl.IntCtrl.u24Reserved          %#x\n",      pVmcb->ctrl.IntCtrl.n.u24Reserved));

        Log4(("ctrl.u64IntShadow                 %#RX64\n",   pVmcb->ctrl.u64IntShadow));
        Log4(("ctrl.u64ExitCode                  %#RX64\n",   pVmcb->ctrl.u64ExitCode));
        Log4(("ctrl.u64ExitInfo1                 %#RX64\n",   pVmcb->ctrl.u64ExitInfo1));
        Log4(("ctrl.u64ExitInfo2                 %#RX64\n",   pVmcb->ctrl.u64ExitInfo2));
        Log4(("ctrl.ExitIntInfo.u8Vector         %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u8Vector));
        Log4(("ctrl.ExitIntInfo.u3Type           %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u3Type));
        Log4(("ctrl.ExitIntInfo.u1ErrorCodeValid %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u1ErrorCodeValid));
        Log4(("ctrl.ExitIntInfo.u19Reserved      %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u19Reserved));
        Log4(("ctrl.ExitIntInfo.u1Valid          %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u1Valid));
        Log4(("ctrl.ExitIntInfo.u32ErrorCode     %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u32ErrorCode));
        Log4(("ctrl.NestedPaging                 %#RX64\n",   pVmcb->ctrl.NestedPaging.u));
        Log4(("ctrl.EventInject.u8Vector         %#x\n",      pVmcb->ctrl.EventInject.n.u8Vector));
        Log4(("ctrl.EventInject.u3Type           %#x\n",      pVmcb->ctrl.EventInject.n.u3Type));
        Log4(("ctrl.EventInject.u1ErrorCodeValid %#x\n",      pVmcb->ctrl.EventInject.n.u1ErrorCodeValid));
        Log4(("ctrl.EventInject.u19Reserved      %#x\n",      pVmcb->ctrl.EventInject.n.u19Reserved));
        Log4(("ctrl.EventInject.u1Valid          %#x\n",      pVmcb->ctrl.EventInject.n.u1Valid));
        Log4(("ctrl.EventInject.u32ErrorCode     %#x\n",      pVmcb->ctrl.EventInject.n.u32ErrorCode));

        Log4(("ctrl.u64NestedPagingCR3           %#RX64\n",   pVmcb->ctrl.u64NestedPagingCR3));
        Log4(("ctrl.u64LBRVirt                   %#RX64\n",   pVmcb->ctrl.u64LBRVirt));

        Log4(("guest.CS.u16Sel                   %RTsel\n",   pVmcb->guest.CS.u16Sel));
        Log4(("guest.CS.u16Attr                  %#x\n",      pVmcb->guest.CS.u16Attr));
        Log4(("guest.CS.u32Limit                 %#RX32\n",   pVmcb->guest.CS.u32Limit));
        Log4(("guest.CS.u64Base                  %#RX64\n",   pVmcb->guest.CS.u64Base));
        Log4(("guest.DS.u16Sel                   %#RTsel\n",  pVmcb->guest.DS.u16Sel));
        Log4(("guest.DS.u16Attr                  %#x\n",      pVmcb->guest.DS.u16Attr));
        Log4(("guest.DS.u32Limit                 %#RX32\n",   pVmcb->guest.DS.u32Limit));
        Log4(("guest.DS.u64Base                  %#RX64\n",   pVmcb->guest.DS.u64Base));
        Log4(("guest.ES.u16Sel                   %RTsel\n",   pVmcb->guest.ES.u16Sel));
        Log4(("guest.ES.u16Attr                  %#x\n",      pVmcb->guest.ES.u16Attr));
        Log4(("guest.ES.u32Limit                 %#RX32\n",   pVmcb->guest.ES.u32Limit));
        Log4(("guest.ES.u64Base                  %#RX64\n",   pVmcb->guest.ES.u64Base));
        Log4(("guest.FS.u16Sel                   %RTsel\n",   pVmcb->guest.FS.u16Sel));
        Log4(("guest.FS.u16Attr                  %#x\n",      pVmcb->guest.FS.u16Attr));
        Log4(("guest.FS.u32Limit                 %#RX32\n",   pVmcb->guest.FS.u32Limit));
        Log4(("guest.FS.u64Base                  %#RX64\n",   pVmcb->guest.FS.u64Base));
        Log4(("guest.GS.u16Sel                   %RTsel\n",   pVmcb->guest.GS.u16Sel));
        Log4(("guest.GS.u16Attr                  %#x\n",      pVmcb->guest.GS.u16Attr));
        Log4(("guest.GS.u32Limit                 %#RX32\n",   pVmcb->guest.GS.u32Limit));
        Log4(("guest.GS.u64Base                  %#RX64\n",   pVmcb->guest.GS.u64Base));

        Log4(("guest.GDTR.u32Limit               %#RX32\n",   pVmcb->guest.GDTR.u32Limit));
        Log4(("guest.GDTR.u64Base                %#RX64\n",   pVmcb->guest.GDTR.u64Base));

        Log4(("guest.LDTR.u16Sel                 %RTsel\n",   pVmcb->guest.LDTR.u16Sel));
        Log4(("guest.LDTR.u16Attr                %#x\n",      pVmcb->guest.LDTR.u16Attr));
        Log4(("guest.LDTR.u32Limit               %#RX32\n",   pVmcb->guest.LDTR.u32Limit));
        Log4(("guest.LDTR.u64Base                %#RX64\n",   pVmcb->guest.LDTR.u64Base));

        Log4(("guest.IDTR.u32Limit               %#RX32\n",   pVmcb->guest.IDTR.u32Limit));
        Log4(("guest.IDTR.u64Base                %#RX64\n",   pVmcb->guest.IDTR.u64Base));

        Log4(("guest.TR.u16Sel                   %RTsel\n",   pVmcb->guest.TR.u16Sel));
        Log4(("guest.TR.u16Attr                  %#x\n",      pVmcb->guest.TR.u16Attr));
        Log4(("guest.TR.u32Limit                 %#RX32\n",   pVmcb->guest.TR.u32Limit));
        Log4(("guest.TR.u64Base                  %#RX64\n",   pVmcb->guest.TR.u64Base));
           4
        Log4(("guest.u8CPL                       %#x\n",      pVmcb->guest.u8CPL));
        Log4(("guest.u64CR0                      %#RX64\n",   pVmcb->guest.u64CR0));
        Log4(("guest.u64CR2                      %#RX64\n",   pVmcb->guest.u64CR2));
        Log4(("guest.u64CR3                      %#RX64\n",   pVmcb->guest.u64CR3));
        Log4(("guest.u64CR4                      %#RX64\n",   pVmcb->guest.u64CR4));
        Log4(("guest.u64DR6                      %#RX64\n",   pVmcb->guest.u64DR6));
        Log4(("guest.u64DR7                      %#RX64\n",   pVmcb->guest.u64DR7));

        Log4(("guest.u64RIP                      %#RX64\n",   pVmcb->guest.u64RIP));
        Log4(("guest.u64RSP                      %#RX64\n",   pVmcb->guest.u64RSP));
        Log4(("guest.u64RAX                      %#RX64\n",   pVmcb->guest.u64RAX));
        Log4(("guest.u64RFlags                   %#RX64\n",   pVmcb->guest.u64RFlags));

        Log4(("guest.u64SysEnterCS               %#RX64\n",   pVmcb->guest.u64SysEnterCS));
        Log4(("guest.u64SysEnterEIP              %#RX64\n",   pVmcb->guest.u64SysEnterEIP));
        Log4(("guest.u64SysEnterESP              %#RX64\n",   pVmcb->guest.u64SysEnterESP));

        Log4(("guest.u64EFER                     %#RX64\n",   pVmcb->guest.u64EFER));
        Log4(("guest.u64STAR                     %#RX64\n",   pVmcb->guest.u64STAR));
        Log4(("guest.u64LSTAR                    %#RX64\n",   pVmcb->guest.u64LSTAR));
        Log4(("guest.u64CSTAR                    %#RX64\n",   pVmcb->guest.u64CSTAR));
        Log4(("guest.u64SFMASK                   %#RX64\n",   pVmcb->guest.u64SFMASK));
        Log4(("guest.u64KernelGSBase             %#RX64\n",   pVmcb->guest.u64KernelGSBase));
        Log4(("guest.u64GPAT                     %#RX64\n",   pVmcb->guest.u64GPAT));
        Log4(("guest.u64DBGCTL                   %#RX64\n",   pVmcb->guest.u64DBGCTL));
        Log4(("guest.u64BR_FROM                  %#RX64\n",   pVmcb->guest.u64BR_FROM));
        Log4(("guest.u64BR_TO                    %#RX64\n",   pVmcb->guest.u64BR_TO));
        Log4(("guest.u64LASTEXCPFROM             %#RX64\n",   pVmcb->guest.u64LASTEXCPFROM));
        Log4(("guest.u64LASTEXCPTO               %#RX64\n",   pVmcb->guest.u64LASTEXCPTO));
#endif
    }
    else
        Log4(("hmR0SvmReportWorldSwitchError: rcVMRun=%d\n", rcVMRun));
}


/**
 * Check per-VM and per-VCPU force flag actions that require us to go back to
 * ring-3 for one reason or another.
 *
 * @returns VBox status code (information status code included).
 * @retval VINF_SUCCESS if we don't have any actions that require going back to
 *         ring-3.
 * @retval VINF_PGM_SYNC_CR3 if we have pending PGM CR3 sync.
 * @retval VINF_EM_PENDING_REQUEST if we have pending requests (like hardware
 *         interrupts)
 * @retval VINF_PGM_POOL_FLUSH_PENDING if PGM is doing a pool flush and requires
 *         all EMTs to be in ring-3.
 * @retval VINF_EM_RAW_TO_R3 if there is pending DMA requests.
 * @retval VINF_EM_NO_MEMORY PGM is out of memory, we need to return
 *         to the EM loop.
 *
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
static int hmR0SvmCheckForceFlags(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(VMMRZCallRing3IsEnabled(pVCpu));

    if (   VM_FF_IS_PENDING(pVM, VM_FF_HM_TO_R3_MASK | VM_FF_REQUEST | VM_FF_PGM_POOL_FLUSH_PENDING | VM_FF_PDM_DMA)
        || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_TO_R3_MASK | VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL
                               | VMCPU_FF_REQUEST | VMCPU_FF_HM_UPDATE_CR3))
    {
        /* Pending HM CR3 sync. No PAE PDPEs (VMCPU_FF_HM_UPDATE_PAE_PDPES) on AMD-V. */
        if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_UPDATE_CR3))
        {
            rc = PGMUpdateCR3(pVCpu, pCtx->cr3);
            Assert(rc == VINF_SUCCESS || rc == VINF_PGM_SYNC_CR3);
            Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_UPDATE_CR3));
        }

        /* Pending PGM C3 sync. */
        if (VMCPU_FF_IS_PENDING(pVCpu,VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL))
        {
            rc = PGMSyncCR3(pVCpu, pCtx->cr0, pCtx->cr3, pCtx->cr4, VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
            if (rc != VINF_SUCCESS)
            {
                AssertRC(rc);
                Log4(("hmR0SvmCheckForceFlags: PGMSyncCR3 forcing us back to ring-3. rc=%d\n", rc));
                return rc;
            }
        }

        /* Pending HM-to-R3 operations (critsects, timers, EMT rendezvous etc.) */
        /* -XXX- what was that about single stepping?  */
        if (   VM_FF_IS_PENDING(pVM, VM_FF_HM_TO_R3_MASK)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
        {
            STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchHmToR3FF);
            rc = RT_UNLIKELY(VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY)) ? VINF_EM_NO_MEMORY : VINF_EM_RAW_TO_R3;
            Log4(("hmR0SvmCheckForceFlags: HM_TO_R3 forcing us back to ring-3. rc=%d\n", rc));
            return rc;
        }

        /* Pending VM request packets, such as hardware interrupts. */
        if (   VM_FF_IS_PENDING(pVM, VM_FF_REQUEST)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_REQUEST))
        {
            Log4(("hmR0SvmCheckForceFlags: Pending VM request forcing us back to ring-3\n"));
            return VINF_EM_PENDING_REQUEST;
        }

        /* Pending PGM pool flushes. */
        if (VM_FF_IS_PENDING(pVM, VM_FF_PGM_POOL_FLUSH_PENDING))
        {
            Log4(("hmR0SvmCheckForceFlags: PGM pool flush pending forcing us back to ring-3\n"));
            return VINF_PGM_POOL_FLUSH_PENDING;
        }

        /* Pending DMA requests. */
        if (VM_FF_IS_PENDING(pVM, VM_FF_PDM_DMA))
        {
            Log4(("hmR0SvmCheckForceFlags: Pending DMA request forcing us back to ring-3\n"));
            return VINF_EM_RAW_TO_R3;
        }
    }

    /* Paranoia. */
    Assert(rc != VERR_EM_INTERPRETER);
    return VINF_SUCCESS;
}


/**
 * Does the preparations before executing guest code in AMD-V.
 *
 * This may cause longjmps to ring-3 and may even result in rescheduling to the
 * recompiler. We must be cautious what we do here regarding committing
 * guest-state information into the the VMCB assuming we assuredly execute the
 * guest in AMD-V. If we fall back to the recompiler after updating the VMCB and
 * clearing the common-state (TRPM/forceflags), we must undo those changes so
 * that the recompiler can (and should) use them when it resumes guest
 * execution. Otherwise such operations must be done when we can no longer
 * exit to ring-3.
 *
 * @returns VBox status code (informational status codes included).
 * @retval VINF_SUCCESS if we can proceed with running the guest.
 * @retval VINF_* scheduling changes, we have to go back to ring-3.
 *
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 */
DECLINE(int) hmR0SvmPreRunGuest(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    /* Check force flag actions that might require us to go back to ring-3. */
    int rc = hmR0SvmCheckForceFlags(pVM, pVCpu, pCtx);
    if (rc != VINF_SUCCESS)
        return rc;

#ifdef VBOX_WITH_VMMR0_DISABLE_PREEMPTION
    /* We disable interrupts so that we don't miss any interrupts that would flag preemption (IPI/timers etc.) */
    pSvmTransient->uEFlags = ASMIntDisableFlags();
    if (RTThreadPreemptIsPending(NIL_RTTHREAD))
    {
        ASMSetFlags(pSvmTransient->uEFlags);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatPendingHostIrq);
        /* Don't use VINF_EM_RAW_INTERRUPT_HYPER as we can't assume the host does kernel preemption. Maybe some day? */
        return VINF_EM_RAW_INTERRUPT;
    }
    VMCPU_ASSERT_STATE(pVCpu, VMCPUSTATE_STARTED_HM);
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC);
#endif

    /* Convert any pending TRPM traps to HM events for injection. */
    /** @todo Optimization: move this before disabling interrupts, restore state
     *        using pVmcb->ctrl.EventInject.u. */
    if (TRPMHasTrap(pVCpu))
        hmR0SvmTrpmTrapToPendingEvent(pVCpu);

    hmR0SvmInjectPendingEvent(pVCpu, pCtx);

    /** @todo -XXX- TPR patching. */
    return VINF_SUCCESS;
}


/**
 * Prepares to run guest code in VT-x and we've committed to doing so. This
 * means there is no backing out to ring-3 or anywhere else at this
 * point.
 *
 * @param   pVM             Pointer to the VM.
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 *
 * @remarks Called with preemption disabled.
 * @remarks No-long-jump zone!!!
 */
DECLINLINE(void) hmR0SvmPreRunGuestCommitted(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));
    Assert(VMMR0IsLogFlushDisabled(pVCpu));

#ifndef VBOX_WITH_VMMR0_DISABLE_PREEMPTION
    /** @todo I don't see the point of this, VMMR0EntryFast() already disables interrupts for the entire period. */
    pSvmTransient->uEFlags = ASMIntDisableFlags();
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC);
#endif

    /* -XXX- todo TPR syncing. */

    /*
     * Re-enable nested paging (automatically disabled on every VM-exit). See AMD spec. 15.25.3 "Enabling Nested Paging".
     * We avoid changing the corresponding VMCB Clean Bit as we're not changing it to a different value since the previous run.
     */
    /** @todo The above assumption could be wrong. It's not documented what
     *        should be done wrt to the VMCB Clean Bit, but we'll find out the
     *        hard way. */
    pVmcb->ctrl.NestedPaging.n.u1NestedPaging = pVM->hm.s.fNestedPaging;

    /* Load the guest state. */
    int rc = SVMR0LoadGuestState(pVM, pVCpu, pCtx);
    AssertRC(rc);
    AssertMsg(!pVCpu->hm.s.fContextUseFlags, ("fContextUseFlags =%#x\n", pVCpu->hm.s.fContextUseFlags));
    STAM_COUNTER_INC(&pVCpu->hm.s.StatLoadFull);

    /* Flush the appropriate tagged-TLB entries. */
    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, true);    /* Used for TLB-shootdowns, set this across the world switch. */
    hmR0SvmFlushTaggedTlb(pVCpu);
    Assert(HMR0GetCurrentCpu()->idCpu == pVCpu->hm.s.idLastCpu);

    TMNotifyStartOfExecution(pVCpu);                            /* Finally, notify TM to resume its clocks as we're about
                                                                    to start executing. */

    /*
     * Save the current Host TSC_AUX and write the guest TSC_AUX to the host, so that
     * RDTSCPs (that don't cause exits) reads the guest MSR. See @bugref{3324}.
     *
     * This should be done -after- any RDTSCPs for obtaining the host timestamp (TM, STAM etc).
     */
    if (    (pVM->hm.s.cpuid.u32AMDFeatureEDX & X86_CPUID_EXT_FEATURE_EDX_RDTSCP)
        && !(pVmcb->ctrl.u32InterceptCtrl2 & SVM_CTRL2_INTERCEPT_RDTSCP))
    {
        pVCpu->hm.s.u64HostTscAux = ASMRdMsr(MSR_K8_TSC_AUX);
        uint64_t u64GuestTscAux = 0;
        rc2 = CPUMQueryGuestMsr(pVCpu, MSR_K8_TSC_AUX, &u64GuestTscAux);
        AssertRC(rc2);
        ASMWrMsr(MSR_K8_TSC_AUX, u64GuestTscAux);
    }
}


/**
 * Wrapper for running the guest code in AMD-V.
 *
 * @returns VBox strict status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
DECLINLINE(int) hmR0SvmRunGuest(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * 64-bit Windows uses XMM registers in the kernel as the Microsoft compiler expresses floating-point operations
     * using SSE instructions. Some XMM registers (XMM6-XMM15) are callee-saved and thus the need for this XMM wrapper.
     * Refer MSDN docs. "Configuring Programs for 64-bit / x64 Software Conventions / Register Usage" for details.
     */
#ifdef VBOX_WITH_KERNEL_USING_XMM
    return HMR0SVMRunWrapXMM(pVCpu->hm.s.svm.HCPhysVmcbHost, pVCpu->hm.s.svm.HCPhysVmcb, pCtx, pVM, pVCpu,
                             pVCpu->hm.s.svm.pfnVMRun);
#else
    return pVCpu->hm.s.svm.pfnVMRun(pVCpu->hm.s.svm.HCPhysVmcbHost, pVCpu->hm.s.svm.HCPhysVmcb, pCtx, pVM, pVCpu);
#endif
}


/**
 * Performs some essential restoration of state after running guest code in
 * AMD-V.
 *
 * @param   pVM             Pointer to the VM.
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   pMixedCtx       Pointer to the guest-CPU context. The data maybe
 *                          out-of-sync. Make sure to update the required fields
 *                          before using them.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 * @param   rcVMRun         Return code of VMRUN.
 *
 * @remarks Called with interrupts disabled.
 * @remarks No-long-jump zone!!! This function will however re-enable longjmps
 *          unconditionally when it is safe to do so.
 */
DECLINLINE(void) hmR0SvmPostRunGuest(PVM pVM, PVMCPU pVCpu, PCPUMCTX pMixedCtx, PSVMTRANSIENT pSvmTransient, rcVMRun)
{
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));

    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, false);   /* See HMInvalidatePageOnAllVCpus(): used for TLB-shootdowns. */
    ASMAtomicIncU32(&pVCpu->hm.s.cWorldSwitchExits);            /* Initialized in vmR3CreateUVM(): used for TLB-shootdowns. */

    PSVMVMCB pVmcb = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;
    pVmcb->ctrl.u64VmcbCleanBits = HMSVM_VMCB_CLEAN_ALL;        /* Mark the VMCB-state cache as unmodified by VMM. */

    /* Restore host's TSC_AUX if required. */
    if (!(pVmcb->ctrl.u32InterceptCtrl1 & SVM_CTRL1_INTERCEPT_RDTSC))
    {
        if (pVM->hm.s.cpuid.u32AMDFeatureEDX & X86_CPUID_EXT_FEATURE_EDX_RDTSCP)
            ASMWrMsr(MSR_K8_TSC_AUX, pVCpu->hm.s.u64HostTscAux);

        /** @todo Find a way to fix hardcoding a guestimate.  */
        TMCpuTickSetLastSeen(pVCpu, ASMReadTSC() +
                             pVmcb->ctrl.u64TSCOffset - 0x400);
    }

    TMNotifyEndOfExecution(pVCpu);                              /* Notify TM that the guest is no longer running. */
    Assert(!(ASMGetFlags() & X86_EFL_IF));
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_HM);

    /* -XXX- TPR patching? */

    ASMSetFlags(pSvmTransient->uEFlags);                        /* Enable interrupts. */

    VMMRZCallRing3SetNotification(pVCpu, hmR0SvmCallRing3Callback, pMixedCtx);
    VMMRZCallRing3Enable(pVCpu);                                /* It is now safe to do longjmps to ring-3!!! */

    pSvmTransient->u64ExitCode = pVmcb->ctrl.u64ExitCode;
    hmR0SvmSaveGuestState(pVCpu, pMixedCtx);                    /* Save the guest state from the VMCB to the guest-CPU context. */

    /* --XXX- TPR syncing todo */

    /* -XXX- premature interruption during event injection */
}


/**
 * Runs the guest code using AMD-V.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   pVCpu       Pointer to the VMCPU.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
VMMR0DECL(int) SVMR0RunGuestCode(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(VMMRZCallRing3IsEnabled(pVCpu));
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    SVMTRANSIENT SvmTransient;
    uint32_t cLoops = 0;
    PSVMVMCB pVmcb  = (PSVMVMCB)pVCpu->hm.s.svm.pvVmcb;
    int      rc     = VERR_INTERNAL_ERROR_5;

    for (;; cLoops++)
    {
        Assert(!HMR0SuspendPending());
        AssertMsg(pVCpu->hm.s.idEnteredCpu == RTMpCpuId(),
                  ("Illegal migration! Entered on CPU %u Current %u cLoops=%u\n", (unsigned)pVCpu->hm.s.idEnteredCpu,
                  (unsigned)RTMpCpuId(), cLoops));

        /* Preparatory work for running guest code, this may return to ring-3 for some last minute updates. */
        STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatEntry, x);
        rc = hmR0SvmPreRunGuest(pVM, pVCpu, pCtx, &SvmTransient);
        if (rc != VINF_SUCCESS)
            break;

        /*
         * No longjmps to ring-3 from this point on!!!
         * Asserts() will still longjmp to ring-3 (but won't return), which is intentional, better than a kernel panic.
         * This also disables flushing of the R0-logger instance (if any).
         */
        VMMRZCallRing3Disable(pVCpu);
        VMMRZCallRing3RemoveNotification(pVCpu);
        hmR0SvmPreRunGuestCommitted(pVM, pVCpu, pCtx, &SvmTransient);

        rc = hmR0SvmRunGuest(pVM, pVCpu, pCtx);

        /*
         * Restore any residual host-state and save any bits shared between host and guest into the guest-CPU state.
         * This will also re-enable longjmps to ring-3 when it has reached a safe point!!!
         */
        hmR0SvmPostRunGuest(pVM, pVCpu, pCtx, &SvmTransient, rc);
        if (RT_UNLIKELY(   rc != VINF_SUCCESS                              /* Check for errors with running the VM (VMRUN). */
                        || SvmTransient.u64ExitCode == SVM_EXIT_INVALID))  /* Check for errors due to invalid guest state. */
        {
            if (rc == VINF_SUCCESS);
                rc = VERR_SVM_INVALID_GUEST_STATE;
            hmR0SvmReportWorldSwitchError(pVM, pVCpu, rc, pCtx, &SvmTransient);
            return rc;
        }

        /* Handle the #VMEXIT. */
        AssertMsg(SvmTransient.u64ExitCode != SVM_EXIT_INVALID, ("%#x\n", SvmTransient.u64ExitCode));
        HMSVM_EXITCODE_STAM_COUNTER_INC(SvmTransient.u64ExitCode);
        rc = hmR0SvmHandleExit(pVCpu, pCtx, &SvmTransient);
        if (rc != VINF_SUCCESS)
            break;
        else if (cLoops > pVM->hm.s.cMaxResumeLoops)
        {
            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitMaxResume);
            rc = VINF_EM_RAW_INTERRUPT;
            break;
        }
    }

    if (rc == VERR_EM_INTERPRETER)
        rc = VINF_EM_RAW_EMULATE_INSTR;
    else if (rc == VINF_EM_RESET)
        rc = VINF_EM_TRIPLE_FAULT;
    hmR0SvmExitToRing3(pVM, pVCpu, pCtx, rc);
    return rc;
}


/**
 * Handles a #VMEXIT (for all EXITCODE values except SVM_EXIT_INVALID).
 *
 * @returns VBox status code (informational status codes included).
 * @param   pVCpu           Pointer to the VMCPU.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 */
DECLINLINE(int) hmR0SvmHandleExit(PVMCPU pVCpu, PCPUMCTX pMixedCtx, PSVMTRANSIENT pSvmTransient)
{
    int rc;
    uint32_t u32ExitCode = pSvmTransient->u64ExitCode;
    switch (u32ExitCode)
    {

    }
    return rc;

}

#ifdef DEBUG
/* Is there some generic IPRT define for this that are not in Runtime/internal/\* ?? */
# define HMSVM_ASSERT_PREEMPT_CPUID_VAR() \
    RTCPUID const idAssertCpu = RTThreadPreemptIsEnabled(NIL_RTTHREAD) ? NIL_RTCPUID : RTMpCpuId()

# define HMSVM_ASSERT_PREEMPT_CPUID() \
   do \
   { \
        RTCPUID const idAssertCpuNow = RTThreadPreemptIsEnabled(NIL_RTTHREAD) ? NIL_RTCPUID : RTMpCpuId(); \
        AssertMsg(idAssertCpu == idAssertCpuNow, ("SVM %#x, %#x\n", idAssertCpu, idAssertCpuNow)); \
   } while (0)

# define HMSVM_VALIDATE_EXIT_HANDLER_PARAMS() \
            do { \
                AssertPtr(pVCpu); \
                AssertPtr(pMixedCtx); \
                AssertPtr(pSvmTransient); \
                Assert(ASMIntAreEnabled()); \
                Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD)); \
                HMSVM_ASSERT_PREEMPT_CPUID_VAR(); \
                Log4Func(("vcpu[%u] -v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-\n", (uint32_t)pVCpu->idCpu)); \
                Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD)); \
                if (VMMR0IsLogFlushDisabled(pVCpu)) \
                    HMSVM_ASSERT_PREEMPT_CPUID(); \
                HMSVM_STOP_EXIT_DISPATCH_PROF(); \
            } while (0)
#else   /* Release builds */
# define HMSVM_VALIDATE_EXIT_HANDLER_PARAMS() do { } while(0)
#endif

