; $Id$
;; @file
; IPRT - No-CRT fegetexcept - AMD64 & X86.
;

;
; Copyright (C) 2022 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL) only, as it comes in the "COPYING.CDDL" file of the
; VirtualBox OSE distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;


%define RT_ASM_WITH_SEH64
%include "iprt/asmdefs.mac"
%include "iprt/x86.mac"


BEGINCODE

;;
; Gets the mask of enabled exceptions, e.g. unmasked (BSD/GNU extension).
;
; @returns  eax = inverted x87/sse exception mask (X86_MXCSR_XCPT_FLAGS).
;           Will not return X86_FSW_SF.
;
RT_NOCRT_BEGINPROC fegetexcept
        push    xBP
        SEH64_PUSH_xBP
        mov     xBP, xSP
        SEH64_SET_FRAME_xBP 0
        sub     xSP, 10h
        SEH64_ALLOCATE_STACK 10h
        SEH64_END_PROLOGUE

        ;
        ; Save control word and isolate the exception mask.
        ;
        ; On 64-bit we'll use the MXCSR since the windows compiler/CRT doesn't
        ; necessarily keep them in sync.  We'll still return the x87-style flags.
        ;
%ifdef RT_ARCH_AMD64
        stmxcsr [xBP - 10h]
        mov     eax, [xBP - 10h]
        shr     eax, X86_MXCSR_XCPT_MASK_SHIFT
%else
        fstcw   [xBP - 10h]
        movzx   eax, word [xBP - 10h]
%endif

        not     eax                         ; Invert it as we return the enabled rather than masked exceptions.
        and     eax, X86_MXCSR_XCPT_FLAGS   ; Use the SSE mask so we don't return X86_FSW_SF here.

.return_val:
        leave
        ret
ENDPROC   RT_NOCRT(fegetexcept)

