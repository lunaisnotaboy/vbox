/* $Id$ */
/** @file
 * Private Shared Clipboard code.
 */

/*
 * Copyright (C) 2023 Oracle and/or its affiliates.
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

#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include "LoggingNew.h"

#include "GuestImpl.h"
#include "AutoCaller.h"

#ifdef VBOX_WITH_SHARED_CLIPBOARD
# include "ConsoleImpl.h"
# include "ProgressImpl.h"
# include "GuestShClPrivate.h"

# include <iprt/semaphore.h>
# include <iprt/cpp/utils.h>

# include <VMMDev.h>

# include <VBox/GuestHost/SharedClipboard.h>
# include <VBox/HostServices/VBoxClipboardSvc.h>
# include <VBox/version.h>


/*********************************************************************************************************************************
 * GuestShCl implementation.                                                                                                     *
 ********************************************************************************************************************************/

/** Static (Singleton) instance of the Shared Clipboard management object. */
GuestShCl* GuestShCl::s_pInstance = NULL;

GuestShCl::GuestShCl(Console *pConsole)
    : m_pConsole(pConsole)
{
    LogFlowFuncEnter();

    RT_ZERO(m_SvcExtVRDP);

    int vrc = RTCritSectInit(&m_CritSect);
    if (RT_FAILURE(vrc))
        throw vrc;
}

GuestShCl::~GuestShCl(void)
{
    uninit();
}

/**
 * Uninitializes the Shared Clipboard management object.
 */
void GuestShCl::uninit(void)
{
    LogFlowFuncEnter();

    if (RTCritSectIsInitialized(&m_CritSect))
        RTCritSectDelete(&m_CritSect);

    RT_ZERO(m_SvcExtVRDP);
}

/**
 * Locks the Shared Clipboard management object.
 *
 * @returns VBox status code.
 */
int GuestShCl::lock(void)
{
    int vrc = RTCritSectEnter(&m_CritSect);
    AssertRC(vrc);
    return vrc;
}

/**
 * Unlocks the Shared Clipboard management object.
 *
 * @returns VBox status code.
 */
int GuestShCl::unlock(void)
{
    int vrc = RTCritSectLeave(&m_CritSect);
    AssertRC(vrc);
    return vrc;
}

/**
 * Registers a Shared Clipboard service extension.
 *
 * @returns VBox status code.
 * @param   pfnExtension        Service extension to register.
 * @param   pvExtension         User-supplied data pointer. Optional.
 */
int GuestShCl::RegisterServiceExtension(PFNHGCMSVCEXT pfnExtension, void *pvExtension)
{
    AssertPtrReturn(pfnExtension, VERR_INVALID_POINTER);

    lock();

    PSHCLSVCEXT pExt = &this->m_SvcExtVRDP; /* Currently we only have one extension only. */

    Assert(pExt->pfnExt == NULL);

    pExt->pfnExt = pfnExtension;
    pExt->pvExt  = pvExtension;

    unlock();

    return VINF_SUCCESS;
}

/**
 * Unregisters a Shared Clipboard service extension.
 *
 * @returns VBox status code.
 * @param   pfnExtension        Service extension to unregister.
 */
int GuestShCl::UnregisterServiceExtension(PFNHGCMSVCEXT pfnExtension)
{
    AssertPtrReturn(pfnExtension, VERR_INVALID_POINTER);

    lock();

    PSHCLSVCEXT pExt = &this->m_SvcExtVRDP; /* Currently we only have one extension only. */

    AssertReturnStmt(pExt->pfnExt == pfnExtension, unlock(), VERR_INVALID_PARAMETER);
    AssertPtr(pExt->pfnExt);

    RT_BZERO(pExt, sizeof(SHCLSVCEXT));

    unlock();

    return VINF_SUCCESS;
}

/**
 * Sends a (blocking) message to the host side of the host service.
 *
 * @returns VBox status code.
 * @param   u32Function         HGCM message ID to send.
 * @param   cParms              Number of parameters to send.
 * @param   paParms             Array of parameters to send. Must match \c cParms.
 */
int GuestShCl::hostCall(uint32_t u32Function, uint32_t cParms, PVBOXHGCMSVCPARM paParms) const
{
    /* Forward the information to the VMM device. */
    AssertPtr(m_pConsole);
    VMMDev *pVMMDev = m_pConsole->i_getVMMDev();
    if (!pVMMDev)
        return VERR_COM_OBJECT_NOT_FOUND;

    return pVMMDev->hgcmHostCall("VBoxSharedClipboard", u32Function, cParms, paParms);
}

/**
 * Reports an error by setting the error info and also informs subscribed listeners.
 *
 * @returns VBox status code.
 * @param   pcszId              ID (name) of the clipboard. Can be NULL if not being used.
 * @param   vrc                 Result code (IPRT-style) to report.
 * @param   pcszMsgFmt          Error message to report.
 * @param   ...                 Format string for \a pcszMsgFmt.
 */
int GuestShCl::reportError(const char *pcszId, int vrc, const char *pcszMsgFmt, ...)
{
    /* pcszId can be NULL. */
    AssertReturn(pcszMsgFmt && *pcszMsgFmt != '\0', E_INVALIDARG);

    va_list va;
    va_start(va, pcszMsgFmt);

    Utf8Str strMsg;
    int const vrc2 = strMsg.printfVNoThrow(pcszMsgFmt, va);
    if (RT_FAILURE(vrc2))
    {
        va_end(va);
        return vrc2;
    }

    va_end(va);

    if (pcszId)
        LogRel(("Shared Clipboard (%s): %s (%Rrc)\n", pcszId, strMsg.c_str(), vrc));
    else
        LogRel(("Shared Clipboard: %s (%Rrc)\n", strMsg.c_str(), vrc));

    m_pConsole->i_onClipboardError(pcszId, strMsg.c_str(), vrc);

    return VINF_SUCCESS;
}

/**
 * Static main dispatcher function to handle callbacks from the Shared Clipboard host service.
 *
 * @returns VBox status code.
 * @param   pvExtension         Pointer to service extension.
 * @param   u32Function         Callback HGCM message ID.
 * @param   pvParms             Pointer to optional data provided for a particular message. Optional.
 * @param   cbParms             Size (in bytes) of \a pvParms.
 */
/* static */
DECLCALLBACK(int) GuestShCl::hgcmDispatcher(void *pvExtension, uint32_t u32Function,
                                            void *pvParms, uint32_t cbParms)
{
    LogFlowFunc(("pvExtension=%p, u32Function=%RU32, pvParms=%p, cbParms=%RU32\n",
                 pvExtension, u32Function, pvParms, cbParms));

    GuestShCl *pThis = reinterpret_cast<GuestShCl*>(pvExtension);
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    int vrc = VINF_SUCCESS;

    switch (u32Function)
    {
# ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        case VBOX_SHCL_GUEST_FN_REPLY:
        {
            vrc = pThis->reportError("foo", VERR_ADDRESS_CONFLICT, "bar");
            break;
        }
# endif
        case VBOX_SHCL_GUEST_FN_ERROR:
            vrc = pThis->reportError("foo", VERR_ADDRESS_CONFLICT, "bar");
            break;

        default:
            break;
    }

    PSHCLSVCEXT const pExt = &pThis->m_SvcExtVRDP; /* Currently we only have one extension only. */

    if (pExt->pfnExt)
    {
        int vrc2 = pExt->pfnExt(pExt->pvExt, u32Function, pvParms, cbParms);
        if (RT_SUCCESS(vrc))
            vrc = vrc2;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc; /* Goes back to host service. */
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD */

