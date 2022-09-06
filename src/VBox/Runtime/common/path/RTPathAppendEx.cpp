/* $Id$ */
/** @file
 * IPRT - RTPathAppendEx
 */

/*
 * Copyright (C) 2009-2022 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/path.h>

#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/errcore.h>
#include <iprt/string.h>

#define RTPATH_TEMPLATE_CPP_H "RTPathAppendEx.cpp.h"
#include "rtpath-expand-template.cpp.h"


/**
 * Figures the length of the root part of the path.
 *
 * @returns length of the root specifier.
 * @retval  0 if none.
 *
 * @param   pszPath         The path to investigate.
 *
 * @remarks Unnecessary root slashes will not be counted. The caller will have
 *          to deal with it where it matters.  (Unlike rtPathRootSpecLen which
 *          counts them.)
 */
static size_t rtPathRootSpecLen2(const char *pszPath)
{
    /* fend of wildlife. */
    if (!pszPath)
        return 0;

    /* Root slash? */
    if (RTPATH_IS_SLASH(pszPath[0]))
    {
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
        /* UNC? */
        if (    RTPATH_IS_SLASH(pszPath[1])
            &&  pszPath[2] != '\0'
            &&  !RTPATH_IS_SLASH(pszPath[2]))
        {
            /* Find the end of the server name. */
            const char *pszEnd = pszPath + 2;
            pszEnd += 2;
            while (   *pszEnd != '\0'
                   && !RTPATH_IS_SLASH(*pszEnd))
                pszEnd++;
            if (RTPATH_IS_SLASH(*pszEnd))
            {
                pszEnd++;
                while (RTPATH_IS_SLASH(*pszEnd))
                    pszEnd++;

                /* Find the end of the share name */
                while (   *pszEnd != '\0'
                       && !RTPATH_IS_SLASH(*pszEnd))
                    pszEnd++;
                if (RTPATH_IS_SLASH(*pszEnd))
                    pszEnd++;
                return pszPath - pszEnd;
            }
        }
#endif
        return 1;
    }

#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
    /* Drive specifier? */
    if (   pszPath[0] != '\0'
        && pszPath[1] == ':'
        && RT_C_IS_ALPHA(pszPath[0]))
    {
        if (RTPATH_IS_SLASH(pszPath[2]))
            return 3;
        return 2;
    }
#endif
    return 0;
}


RTDECL(int) RTPathAppendEx(char *pszPath, size_t cbPathDst, const char *pszAppend, size_t cchAppendMax, uint32_t fFlags)
{
    char *pszPathEnd = RTStrEnd(pszPath, cbPathDst);
    AssertReturn(pszPathEnd, VERR_INVALID_PARAMETER);
    Assert(RTPATH_STR_F_IS_VALID(fFlags, 0));

    /*
     * Special cases.
     */
    if (!pszAppend)
        return VINF_SUCCESS;
    size_t cchAppend = RTStrNLen(pszAppend, cchAppendMax);
    if (!cchAppend)
        return VINF_SUCCESS;
    if (pszPathEnd == pszPath)
    {
        if (cchAppend >= cbPathDst)
            return VERR_BUFFER_OVERFLOW;
        memcpy(pszPath, pszAppend, cchAppend);
        pszPath[cchAppend] = '\0';
        return VINF_SUCCESS;
    }

    /*
     * Go to path style specific code now.
     */
    switch (fFlags & RTPATH_STR_F_STYLE_MASK)
    {
#if RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS
        case RTPATH_STR_F_STYLE_HOST:
#endif
        case RTPATH_STR_F_STYLE_DOS:
            return rtPathAppendExStyleDos(pszPath, cbPathDst, pszPathEnd, pszAppend, cchAppend);

#if RTPATH_STYLE != RTPATH_STR_F_STYLE_DOS
        case RTPATH_STR_F_STYLE_HOST:
#endif
        case RTPATH_STR_F_STYLE_UNIX:
            return rtPathAppendExStyleUnix(pszPath, cbPathDst, pszPathEnd, pszAppend, cchAppend);

        default:
            AssertFailedReturn(VERR_INVALID_FLAGS); /* impossible */
    }
}

