/* $Id$ */
/** @file
 * VBoxSeamless - Display notifications
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef GA_INCLUDED_SRC_WINNT_VBoxTray_VBoxTrayInternal_h
#define GA_INCLUDED_SRC_WINNT_VBoxTray_VBoxTrayInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* caps API */
#define VBOXCAPS_ENTRY_IDX_SEAMLESS  0
#define VBOXCAPS_ENTRY_IDX_GRAPHICS  1
#define VBOXCAPS_ENTRY_IDX_COUNT     2

typedef enum VBOXCAPS_ENTRY_FUNCSTATE
{
    /* the cap is unsupported */
    VBOXCAPS_ENTRY_FUNCSTATE_UNSUPPORTED = 0,
    /* the cap is supported */
    VBOXCAPS_ENTRY_FUNCSTATE_SUPPORTED,
    /* the cap functionality is started, it can be disabled however if its AcState is not ACQUIRED */
    VBOXCAPS_ENTRY_FUNCSTATE_STARTED,
} VBOXCAPS_ENTRY_FUNCSTATE;


int VBoxAcquireGuestCaps(uint32_t fOr, uint32_t fNot, bool fCfg);
void VBoxCapsEntryFuncStateSet(uint32_t iCup, VBOXCAPS_ENTRY_FUNCSTATE enmFuncState);
int VBoxCapsInit();
int VBoxCapsReleaseAll();
void VBoxCapsTerm();
BOOL VBoxCapsEntryIsAcquired(uint32_t iCap);
BOOL VBoxCapsEntryIsEnabled(uint32_t iCap);
BOOL VBoxCapsCheckTimer(WPARAM wParam);
int VBoxCapsEntryRelease(uint32_t iCap);
int VBoxCapsEntryAcquire(uint32_t iCap);
int VBoxCapsAcquireAllSupported();


/* console-related caps API */
BOOL VBoxConsoleIsAllowed();
void VBoxConsoleEnable(BOOL fEnable);
void VBoxConsoleCapSetSupported(uint32_t iCap, BOOL fSupported);


/*
 * Dt (desktop [state] tracking) functionality API
 *
 * !!!NOTE: this API is NOT thread-safe!!!
 * */
int vboxDtInit();
void vboxDtTerm();
/* @returns true on "IsInputDesktop" state change */
BOOL vboxDtHandleEvent();
/* @returns true iff the application (VBoxTray) desktop is input */
BOOL vboxDtIsInputDesktop();
HANDLE vboxDtGetNotifyEvent();
BOOL vboxDtCheckTimer(WPARAM wParam);
void VBoxTrayCheckDt();


/*
 * St (session [state] tracking) functionality API
 *
 * !!!NOTE: this API is NOT thread-safe!!!
 * it is supposed to be called & used from within the window message handler thread
 * of the window passed to vboxStInit */
int vboxStInit(HWND hWnd);
void vboxStTerm(void);
/* @returns true on "IsActiveConsole" state change */
BOOL vboxStHandleEvent(WPARAM EventID);
BOOL vboxStIsActiveConsole();
BOOL vboxStCheckTimer(WPARAM wEvent);


DWORD VBoxDisplayGetCount();
DWORD VBoxDisplayGetConfig(const DWORD NumDevices, DWORD *pDevPrimaryNum, DWORD *pNumDevices, DISPLAY_DEVICE *paDisplayDevices, DEVMODE *paDeviceModes);

DWORD EnableAndResizeDispDev(DEVMODE *paDeviceModes, DISPLAY_DEVICE *paDisplayDevices, DWORD totalDispNum, UINT Id, DWORD aWidth, DWORD aHeight,
                             DWORD aBitsPerPixel, LONG aPosX, LONG aPosY, BOOL fEnabled, BOOL fExtDispSup);

#endif /* !GA_INCLUDED_SRC_WINNT_VBoxTray_VBoxTrayInternal_h */
