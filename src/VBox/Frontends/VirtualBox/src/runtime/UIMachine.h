/* $Id$ */
/** @file
 * VBox Qt GUI - UIMachine class declaration.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_runtime_UIMachine_h
#define FEQT_INCLUDED_SRC_runtime_UIMachine_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>
#include <QPixmap>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UIMachineDefs.h"
#include "UIMousePointerShapeData.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMediumAttachment.h"

/* Forward declarations: */
class QWidget;
class UIActionPool;
class UISession;
class UIMachineLogic;
#ifdef VBOX_WS_MAC
 class QMenuBar;
 class QTimer;
#endif

/** Singleton QObject extension
  * used as virtual machine (VM) singleton instance. */
class UIMachine : public QObject
{
    Q_OBJECT;

signals:

    /** Requests async visual-state change. */
    void sigRequestAsyncVisualStateChange(UIVisualStateType visualStateType);

    /** @name COM events stuff.
     ** @{ */
        /** Notifies about additions state change. */
        void sigAdditionsStateChange();
        /** Notifies about additions state actually change. */
        void sigAdditionsStateActualChange();
        /** Notifies about audio adapter change. */
        void sigAudioAdapterChange();
        /** Notifies about clipboard mode change. */
        void sigClipboardModeChange(KClipboardMode enmMode);
        /** Notifies about CPU execution cap change. */
        void sigCPUExecutionCapChange();
        /** Notifies about DnD mode change. */
        void sigDnDModeChange(KDnDMode enmMode);
        /** Notifies about guest monitor change. */
        void sigGuestMonitorChange(KGuestMonitorChangedEventType emnChangeType, ulong uScreenId, QRect screenGeo);
        /** Notifies about machine change. */
        void sigMachineStateChange();
        /** Notifies about medium change. */
        void sigMediumChange(const CMediumAttachment &comMediumAttachment);
        /** Notifies about network adapter change. */
        void sigNetworkAdapterChange(const CNetworkAdapter &comNetworkAdapter);
        /** Notifies about recording change. */
        void sigRecordingChange();
        /** Notifies about shared folder change. */
        void sigSharedFolderChange();
        /** Handles storage device change signal. */
        void sigStorageDeviceChange(const CMediumAttachment &comAttachment, bool fRemoved, bool fSilent);
        /** Handles USB controller change signal. */
        void sigUSBControllerChange();
        /** Handles USB device state change signal. */
        void sigUSBDeviceStateChange(const CUSBDevice &comDevice, bool fAttached, const CVirtualBoxErrorInfo &comError);
        /** Notifies about VRDE change. */
        void sigVRDEChange();

        /** Notifies about runtime error happened. */
        void sigRuntimeError(bool bIsFatal, const QString &strErrorId, const QString &strMessage);

#ifdef VBOX_WS_MAC
        /** Notifies about VM window should be shown. */
        void sigShowWindows();
#endif
    /** @} */

    /** @name Keyboard stuff.
     ** @{ */
        /** Notifies about keyboard LEDs change. */
        void sigKeyboardLedsChange();

        /** Notifies listeners about keyboard state-change. */
        void sigKeyboardStateChange(int iState);
    /** @} */

    /** @name Mouse cursor stuff.
     ** @{ */
        /** Notifies listeners about mouse pointer shape change. */
        void sigMousePointerShapeChange();
        /** Notifies listeners about mouse capability change. */
        void sigMouseCapabilityChange();
        /** Notifies listeners about cursor position change. */
        void sigCursorPositionChange();

        /** Notifies listeners about mouse state-change. */
        void sigMouseStateChange(int iState);
    /** @} */

    /** @name Host-screen stuff.
     ** @{ */
        /** Notifies about host-screen count change. */
        void sigHostScreenCountChange();
        /** Notifies about host-screen geometry change. */
        void sigHostScreenGeometryChange();
        /** Notifies about host-screen available-area change. */
        void sigHostScreenAvailableAreaChange();
    /** @} */

public:

    /** Static factory to start machine with passed @a uID.
      * @return true if machine was started, false otherwise. */
    static bool startMachine(const QUuid &uID);
    /** Static constructor. */
    static bool create();
    /** Static destructor. */
    static void destroy();
    /** Static instance. */
    static UIMachine *instance() { return s_pInstance; }

    /** Returns session UI instance. */
    UISession *uisession() const { return m_pSession; }
    /** Returns machine-logic instance. */
    UIMachineLogic* machineLogic() const { return m_pMachineLogic; }
    /** Returns active machine-window reference (if possible). */
    QWidget* activeWindow() const;

    /** Returns whether requested visual @a state allowed. */
    bool isVisualStateAllowed(UIVisualStateType state) const { return m_allowedVisualStates & state; }

    /** Requests async visual-state change. */
    void asyncChangeVisualState(UIVisualStateType visualStateType);

    /** Requests visual-state to be entered when possible. */
    void setRequestedVisualState(UIVisualStateType visualStateType);
    /** Returns requested visual-state to be entered when possible. */
    UIVisualStateType requestedVisualState() const;

    /** @name Branding stuff.
     ** @{ */
        /** Returns the cached machine-window icon. */
        QIcon *machineWindowIcon() const { return m_pMachineWindowIcon; }
#ifndef VBOX_WS_MAC
        /** Returns redefined machine-window name postfix. */
        QString machineWindowNamePostfix() const { return m_strMachineWindowNamePostfix; }
#endif
    /** @} */

    /** @name Actions stuff.
     ** @{ */
        /** Returns the action-pool instance. */
        UIActionPool *actionPool() const { return m_pActionPool; }

        /** Updates additions actions state. */
        void updateStateAdditionsActions();
        /** Updates Audio action state. */
        void updateStateAudioActions();
        /** Updates Recording action state. */
        void updateStateRecordingAction();
        /** Updates VRDE server action state. */
        void updateStateVRDEServerAction();
    /** @} */

    /** @name Host-screen stuff.
     ** @{ */
        /** Returns the list of host-screen geometries we currently have. */
        QList<QRect> hostScreens() const { return m_hostScreens; }
    /** @} */

    /** @name Guest-screen stuff.
     ** @{ */
        /** Returns whether guest-screen with @a uScreenId specified is expected to be visible. */
        bool isScreenVisibleHostDesires(ulong uScreenId) const;
        /** Defines whether guest-screen with @a uScreenId specified is expected to be @a fVisible. */
        void setScreenVisibleHostDesires(ulong uScreenId, bool fVisible);

        /** Returns whether guest-screen with @a uScreenId specified is actually visible. */
        bool isScreenVisible(ulong uScreenId) const;
        /** Defines whether guest-screen with @a uScreenId specified is actually @a fVisible. */
        void setScreenVisible(ulong uScreenId, bool fIsMonitorVisible);

        /** Returns a number of visible guest-windows. */
        int countOfVisibleWindows();
        /** Returns the list of visible guest-windows. */
        QList<int> listOfVisibleWindows() const;

        /** Returns last full-screen size for guest-screen with index @a uScreenId. */
        QSize lastFullScreenSize(ulong uScreenId) const;
        /** Defines last full-screen @a size for guest-screen with index @a uScreenId. */
        void setLastFullScreenSize(ulong uScreenId, QSize size);
    /** @} */

    /** @name Keyboard stuff.
     ** @{ */
        /** Returns the NUM lock status. */
        bool isNumLock() const { return m_fNumLock; }
        /** Returns the CAPS lock status. */
        bool isCapsLock() const { return m_fCapsLock; }
        /** Returns the SCROLL lock status. */
        bool isScrollLock() const { return m_fScrollLock; }

        /** Returns the NUM lock adaption count. */
        uint numLockAdaptionCnt() const { return m_uNumLockAdaptionCnt; }
        /** Defines the NUM lock adaption @a uCount. */
        void setNumLockAdaptionCnt(uint uCount) { m_uNumLockAdaptionCnt = uCount; }

        /** Returns the CAPS lock adaption count. */
        uint capsLockAdaptionCnt() const { return m_uCapsLockAdaptionCnt; }
        /** Defines the CAPS lock adaption @a uCount. */
        void setCapsLockAdaptionCnt(uint uCount) { m_uCapsLockAdaptionCnt = uCount; }

        /** Returns whether VM should perform HID LEDs synchronization. */
        bool isHidLedsSyncEnabled() const { return m_fIsHidLedsSyncEnabled; }

        /** Returns the keyboard-state. */
        int keyboardState() const { return m_iKeyboardState; }
    /** @} */

    /** @name Mouse cursor stuff.
     ** @{ */
        /** Returns whether we should hide host mouse pointer. */
        bool isHidingHostPointer() const { return m_fIsHidingHostPointer; }
        /** Returns whether there is valid mouse pointer shape present. */
        bool isValidPointerShapePresent() const { return m_fIsValidPointerShapePresent; }
        /** Returns whether the @a cursorPosition() is valid and could be used by the GUI now. */
        bool isValidCursorPositionPresent() const { return m_fIsValidCursorPositionPresent; }

        /** Returns whether mouse supports absolute coordinates. */
        bool isMouseSupportsAbsolute() const { return m_fIsMouseSupportsAbsolute; }
        /** Returns whether mouse supports relative coordinates. */
        bool isMouseSupportsRelative() const { return m_fIsMouseSupportsRelative; }
        /** Returns whether touch screen is supported. */
        bool isMouseSupportsTouchScreen() const { return m_fIsMouseSupportsTouchScreen; }
        /** Returns whether touch pad is supported. */
        bool isMouseSupportsTouchPad() const { return m_fIsMouseSupportsTouchPad; }
        /** Returns whether guest requires host cursor to be shown. */
        bool isMouseHostCursorNeeded() const { return m_fIsMouseHostCursorNeeded; }

        /** Returns whether mouse is captured. */
        bool isMouseCaptured() const { return m_fIsMouseCaptured; }
        /** Returns whether mouse is integrated. */
        bool isMouseIntegrated() const { return m_fIsMouseIntegrated; }
        /** Defines whether mouse is @a fCaptured. */
        void setMouseCaptured(bool fCaptured) { m_fIsMouseCaptured = fCaptured; }
        /** Defines whether mouse is @a fIntegrated. */
        void setMouseIntegrated(bool fIntegrated) { m_fIsMouseIntegrated = fIntegrated; }

        /** Returns currently cached mouse cursor shape pixmap. */
        QPixmap cursorShapePixmap() const { return m_cursorShapePixmap; }
        /** Returns currently cached mouse cursor mask pixmap. */
        QPixmap cursorMaskPixmap() const { return m_cursorMaskPixmap; }
        /** Returns currently cached mouse cursor size. */
        QSize cursorSize() const { return m_cursorSize; }
        /** Returns currently cached mouse cursor hotspot. */
        QPoint cursorHotspot() const { return m_cursorHotspot; }
        /** Returns currently cached mouse cursor position. */
        QPoint cursorPosition() const { return m_cursorPosition; }

        /** Returns mouse-state. */
        int mouseState() const { return m_iMouseState; }
    /** @} */

    /** @name Close stuff.
     * @{ */
        /** Returns default close action. */
        MachineCloseAction defaultCloseAction() const { return m_defaultCloseAction; }
        /** Returns merged restricted close actions. */
        MachineCloseAction restrictedCloseActions() const { return m_restrictedCloseActions; }
    /** @} */

public slots:

    /** @name Keyboard stuff.
     ** @{ */
        /** Defines @a iKeyboardState. */
        void setKeyboardState(int iKeyboardState) { m_iKeyboardState = iKeyboardState; emit sigKeyboardStateChange(m_iKeyboardState); }
    /** @} */

    /** @name Mouse cursor stuff.
     ** @{ */
        /** Defines @a iMouseState. */
        void setMouseState(int iMouseState) { m_iMouseState = iMouseState; emit sigMouseStateChange(m_iMouseState); }
    /** @} */

    /** @name Close stuff.
     * @{ */
        /** Closes Runtime UI. */
        void closeRuntimeUI();
    /** @} */

private slots:

    /** Visual state-change handler. */
    void sltChangeVisualState(UIVisualStateType visualStateType);

    /** @name COM events stuff.
     ** @{ */
        /** Handles additions state actual change signal. */
        void sltHandleAdditionsActualChange();
        /** Handles audio adapter change signal. */
        void sltHandleAudioAdapterChange();
        /** Handles recording change signal. */
        void sltHandleRecordingChange();
        /** Handles storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
        void sltHandleStorageDeviceChange(const CMediumAttachment &comAttachment, bool fRemoved, bool fSilent);
        /** Handles VRDE change signal. */
        void sltHandleVRDEChange();
    /** @} */

    /** @name Actions stuff.
     ** @{ */
#ifdef VBOX_WS_MAC
        /** Mac OS X: Handles menu-bar configuration-change. */
        void sltHandleMenuBarConfigurationChange(const QUuid &uMachineID);
#endif
    /** @} */

    /** @name Host-screen stuff.
     * @{ */
        /** Handles host-screen count change. */
        void sltHandleHostScreenCountChange();
        /** Handles host-screen geometry change. */
        void sltHandleHostScreenGeometryChange();
        /** Handles host-screen available-area change. */
        void sltHandleHostScreenAvailableAreaChange();

#ifdef VBOX_WS_MAC
        /** MacOS X: Restarts display-reconfiguration watchdog timer from the beginning.
          * @note Watchdog is trying to determine display reconfiguration in
          *       UISession::sltCheckIfHostDisplayChanged() slot every 500ms for 40 tries. */
        void sltHandleHostDisplayAboutToChange();
        /** MacOS X: Determines display reconfiguration.
          * @note Calls for UISession::sltHandleHostScreenCountChange() if screen count changed.
          * @note Calls for UISession::sltHandleHostScreenGeometryChange() if screen geometry changed. */
        void sltCheckIfHostDisplayChanged();
#endif /* VBOX_WS_MAC */
    /** @} */

    /** @name Guest-screen stuff.
     * @{ */
        /** Handles guest-monitor state change. */
        void sltHandleGuestMonitorChange(KGuestMonitorChangedEventType enmChangeType, ulong uScreenId, QRect screenGeo);
    /** @} */

    /** @name Keyboard stuff.
     ** @{ */
        /** Handles signal about keyboard LEDs change.
          * @param  fNumLock     Brings NUM lock status.
          * @param  fCapsLock    Brings CAPS lock status.
          * @param  fScrollLock  Brings SCROLL lock status. */
        void sltHandleKeyboardLedsChange(bool fNumLock, bool fCapsLock, bool fScrollLock);

        /** Handles signal about keyboard LEDs sync state change.
          * @param  fEnabled  Brings sync status. */
        void sltHidLedsSyncStateChanged(bool fEnabled) { m_fIsHidLedsSyncEnabled = fEnabled; }
    /** @} */

    /** @name Mouse cursor stuff.
     ** @{ */
        /** Handles signal about mouse pointer shape data change.
          * @param  shapeData  Brings complex struct describing mouse pointer shape aspects. */
        void sltMousePointerShapeChange(const UIMousePointerShapeData &shapeData);
        /** Handles signal about mouse capability change.
          * @param  fSupportsAbsolute     Brings whether mouse supports absolute coordinates.
          * @param  fSupportsRelative     Brings whether mouse supports relative coordinates.
          * @param  fSupportsTouchScreen  Brings whether touch screen is supported.
          * @param  fSupportsTouchPad     Brings whether touch pad is supported.
          * @param  fNeedsHostCursor      Brings whether guest requires host cursor to be shown. */
        void sltMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative,
                                      bool fSupportsTouchScreen, bool fSupportsTouchPad,
                                      bool fNeedsHostCursor);
        /** Handles signal about guest request to change the cursor position to @a uX * @a uY.
          * @param  fContainsData  Brings whether the @a uX and @a uY values are valid and could be used by the GUI now.
          * @param  uX             Brings cursor position X origin.
          * @param  uY             Brings cursor position Y origin. */
        void sltCursorPositionChange(bool fContainsData,
                                     unsigned long uX,
                                     unsigned long uY);
    /** @} */

private:

    /** Constructor. */
    UIMachine();
    /** Destructor. */
    ~UIMachine();

    /** Prepare routine. */
    bool prepare();
    /** Prepare routine: Session connection stuff. */
    void prepareSessionConnections();
    /** Prepare routine: Screens stuff. */
    void prepareScreens();
    /** Prepare routine: Branding. */
    void prepareBranding();
    /** Prepare routine: Actions stuff. */
    void prepareActions();
    /** Prepare routine: Keyboard stuff. */
    void prepareKeyboard();
    /** Prepare routine: Close stuff. */
    void prepareClose();
    /** Prepare routine: Machine-logic stuff. */
    void prepareMachineLogic();

    /** Cleanup routine: Machine-logic stuff. */
    void cleanupMachineLogic();
    /** Cleanup routine: Actions stuff. */
    void cleanupActions();
    /** Cleanup routine: Branding. */
    void cleanupBranding();
    /** Cleanup routine: Screens stuff. */
    void cleanupScreens();
    /** Cleanup routine: Session stuff. */
    void cleanupSession();
    /** Cleanup routine. */
    void cleanup();

    /** Moves VM to initial state. */
    void enterInitialVisualState();

    /** @name Actions stuff.
     ** @{ */
        /** Updates action restrictions. */
        void updateActionRestrictions();

#ifdef VBOX_WS_MAC
        /** Mac OS X: Updates menu-bar content. */
        void updateMenu();
#endif
    /** @} */

    /** @name Host-screen stuff.
     * @{ */
        /** Update host-screen data. */
        void updateHostScreenData();
    /** @} */

    /** @name Mouse cursor stuff.
     ** @{ */
        /** Updates mouse pointer shape. */
        void updateMousePointerShape();

        /** Updates mouse states. */
        void updateMouseState();

#if defined(VBOX_WS_X11) || defined(VBOX_WS_MAC)
        /** Generate a BGRA bitmap which approximates a XOR/AND mouse pointer.
          *
          * Pixels which has 1 in the AND mask and not 0 in the XOR mask are replaced by
          * the inverted pixel and 8 surrounding pixels with the original color.
          * Fort example a white pixel (W) is replaced with a black (B) pixel:
          *         WWW
          *  W   -> WBW
          *         WWW
          * The surrounding pixels are written only if the corresponding source pixel
          * does not affect the screen, i.e. AND bit is 1 and XOR value is 0. */
        static void renderCursorPixels(const uint32_t *pu32XOR, const uint8_t *pu8AND,
                                       uint32_t u32Width, uint32_t u32Height,
                                       uint32_t *pu32Pixels, uint32_t cbPixels);
#endif /* VBOX_WS_X11 || VBOX_WS_MAC */

#ifdef VBOX_WS_WIN
        static bool isPointer1bpp(const uint8_t *pu8XorMask,
                                  uint uWidth,
                                  uint uHeight);
#endif /* VBOX_WS_WIN */
    /** @} */

    /** Holds the static instance. */
    static UIMachine *s_pInstance;

    /** Holds the session UI instance. */
    UISession *m_pSession;

    /** Holds allowed visual states. */
    UIVisualStateType m_allowedVisualStates;
    /** Holds initial visual state. */
    UIVisualStateType m_initialVisualState;
    /** Holds current visual state. */
    UIVisualStateType m_visualState;
    /** Holds visual state which should be entered when possible. */
    UIVisualStateType m_enmRequestedVisualState;
    /** Holds current machine-logic. */
    UIMachineLogic *m_pMachineLogic;

    /** @name Branding stuff.
     ** @{ */
        /** Holds the cached machine-window icon. */
        QIcon *m_pMachineWindowIcon;

#ifndef VBOX_WS_MAC
        /** Holds redefined machine-window name postfix. */
        QString m_strMachineWindowNamePostfix;
#endif
    /** @} */

    /** @name Actions stuff.
     ** @{ */
        /** Holds the action-pool instance. */
        UIActionPool *m_pActionPool;

#ifdef VBOX_WS_MAC
        /** Mac OS X: Holds the menu-bar instance. */
        QMenuBar *m_pMenuBar;
#endif
    /** @} */

    /** @name Host-screen stuff.
     * @{ */
        /** Holds the list of host-screen geometries we currently have. */
        QList<QRect>  m_hostScreens;

#ifdef VBOX_WS_MAC
        /** Mac OS X: Watchdog timer looking for display reconfiguration. */
        QTimer *m_pWatchdogDisplayChange;
#endif
    /** @} */

    /** @name Guest-screen stuff.
     * @{ */
        /** Holds the list of desired guest-screen visibility flags. */
        QVector<bool>  m_monitorVisibilityVectorHostDesires;
        /** Holds the list of actual guest-screen visibility flags. */
        QVector<bool>  m_monitorVisibilityVector;

        /** Holds the list of guest-screen full-screen sizes. */
        QVector<QSize>  m_monitorLastFullScreenSizeVector;
    /** @} */

    /** @name Keyboard stuff.
     ** @{ */
        /** Holds the NUM lock status. */
        bool  m_fNumLock;
        /** Holds the CAPS lock status. */
        bool  m_fCapsLock;
        /** Holds the SCROLL lock status. */
        bool  m_fScrollLock;

        /** Holds the NUM lock adaption count. */
        uint  m_uNumLockAdaptionCnt;
        /** Holds the CAPS lock adaption count. */
        uint  m_uCapsLockAdaptionCnt;

        /** Holds whether VM should perform HID LEDs synchronization. */
        bool m_fIsHidLedsSyncEnabled;

        /** Holds the keyboard-state. */
        int  m_iKeyboardState;
    /** @} */

    /** @name Mouse cursor stuff.
     ** @{ */
        /** Holds whether we should hide host mouse pointer. */
        bool  m_fIsHidingHostPointer;
        /** Holds whether there is valid mouse pointer shape present. */
        bool  m_fIsValidPointerShapePresent;
        /** Holds whether the @a m_cursorPosition is valid and could be used by the GUI now. */
        bool  m_fIsValidCursorPositionPresent;

        /** Holds whether mouse supports absolute coordinates. */
        bool  m_fIsMouseSupportsAbsolute;
        /** Holds whether mouse supports relative coordinates. */
        bool  m_fIsMouseSupportsRelative;
        /** Holds whether touch screen is supported. */
        bool  m_fIsMouseSupportsTouchScreen;
        /** Holds whether touch pad is supported. */
        bool  m_fIsMouseSupportsTouchPad;
        /** Holds whether guest requires host cursor to be shown. */
        bool  m_fIsMouseHostCursorNeeded;

        /** Holds whether mouse is captured. */
        bool  m_fIsMouseCaptured;
        /** Holds whether mouse is integrated. */
        bool  m_fIsMouseIntegrated;

        /** Holds the mouse pointer shape data. */
        UIMousePointerShapeData  m_shapeData;

        /** Holds cached mouse cursor shape pixmap. */
        QPixmap  m_cursorShapePixmap;
        /** Holds cached mouse cursor mask pixmap. */
        QPixmap  m_cursorMaskPixmap;
        /** Holds cached mouse cursor size. */
        QSize    m_cursorSize;
        /** Holds cached mouse cursor hotspot. */
        QPoint   m_cursorHotspot;
        /** Holds cached mouse cursor position. */
        QPoint   m_cursorPosition;

        /** Holds the mouse-state. */
        int  m_iMouseState;
    /** @} */

    /** @name Close stuff.
     * @{ */
        /** Default close action. */
        MachineCloseAction  m_defaultCloseAction;
        /** Merged restricted close actions. */
        MachineCloseAction  m_restrictedCloseActions;
    /** @} */
};

#define gpMachine UIMachine::instance()

#endif /* !FEQT_INCLUDED_SRC_runtime_UIMachine_h */
