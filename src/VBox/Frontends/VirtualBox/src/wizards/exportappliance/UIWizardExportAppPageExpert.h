/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageExpert class declaration.
 */

/*
 * Copyright (C) 2009-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizardPage.h"
#include "UIWizardExportApp.h"

/* COM includes: */
#include "COMEnums.h"
#include "CAppliance.h"
#include "CCloudClient.h"
#include "CCloudProfile.h"
#include "CVirtualSystemDescription.h"
#include "CVirtualSystemDescriptionForm.h"

/* Forward declarations: */
class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QGridLayout;
class QGroupBox;
class QLabel;
class QListWidget;
class QStackedLayout;
class QStackedWidget;
class QIComboBox;
class QIToolButton;
class UIApplianceExportEditorWidget;
class UIEmptyFilePathSelector;
class UIFormEditorWidget;
class UIWizardExportApp;

/** UIWizardPage extension for UIWizardExportAppPage1, UIWizardExportAppPage2 and UIWizardExportAppPage3. */
class UIWizardExportAppPageExpert : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs expert page.
      * @param  selectedVMNames  Brings the list of selected VM names. */
    UIWizardExportAppPageExpert(const QStringList &selectedVMNames, bool fExportToOCIByDefault);

protected:

    /** Returns wizard this page belongs to. */
    UIWizardExportApp *wizard() const;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs page initialization. */
    virtual void initializePage() /* override */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override */;

    /** Performs page validation. */
    virtual bool validatePage() /* override */;

private slots:

    /** Handles VM selector index change. */
    void sltHandleVMItemSelectionChanged();

    /** Handles format combo change. */
    void sltHandleFormatComboChange();

    /** Handles change in file-name selector. */
    void sltHandleFileSelectorChange();

    /** Handles change in MAC address export policy combo-box. */
    void sltHandleMACAddressExportPolicyComboChange();

    /** Handles change in manifest check-box. */
    void sltHandleManifestCheckBoxChange();

    /** Handles change in include ISOs check-box. */
    void sltHandleIncludeISOsCheckBoxChange();

    /** Handles change in profile combo-box. */
    void sltHandleProfileComboChange();

    /** Handles cloud export radio-button clicked. */
    void sltHandleRadioButtonToggled(QAbstractButton *pButton, bool fToggled);

    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();

private:

    /** Updates machines. */
    void updateMachines();
    /** Updates format. */
    void updateFormat();
    /** Updates profile. */
    void updateProfile();
    /** Updates cloud stuff. */
    void updateCloudStuff();

    /** Holds the list of selected VM names. */
    const QStringList  m_selectedVMNames;
    /** Holds whether default format should be Export to OCI. */
    bool               m_fExportToOCIByDefault;

    /** Holds the default appliance name. */
    QString  m_strDefaultApplianceName;
    /** Holds the file selector name. */
    QString  m_strFileSelectorName;
    /** Holds the file selector ext. */
    QString  m_strFileSelectorExt;

    /** Holds the Cloud Profile object reference. */
    CCloudProfile  m_comCloudProfile;


    /** Holds the VM selector container instance. */
    QGroupBox   *m_pSelectorCnt;

    /** Holds the VM selector instance. */
    QListWidget *m_pVMSelector;


    /** Holds the appliance widget container reference. */
    QGroupBox   *m_pApplianceCnt;

    /** Holds the settings container layout instance. */
    QStackedLayout *m_pSettingsLayout;

    /** Holds the appliance widget reference. */
    UIApplianceExportEditorWidget *m_pApplianceWidget;
    /** Holds the Form Editor widget instance. */
    UIFormEditorWidget            *m_pFormEditor;


    /** Holds the settings widget container reference. */
    QGroupBox *m_pSettingsCnt;

    /** Holds the format layout. */
    QGridLayout *m_pFormatLayout;
    /** Holds the format combo-box label instance. */
    QLabel      *m_pFormatComboBoxLabel;
    /** Holds the format combo-box instance. */
    QIComboBox  *m_pFormatComboBox;

    /** Holds the settings widget 1 instance. */
    QStackedWidget *m_pSettingsWidget1;

    /** Holds the settings layout 1. */
    QGridLayout             *m_pSettingsLayout1;
    /** Holds the file selector label instance. */
    QLabel                  *m_pFileSelectorLabel;
    /** Holds the file selector instance. */
    UIEmptyFilePathSelector *m_pFileSelector;
    /** Holds the MAC address policy combo-box label instance. */
    QLabel                  *m_pMACComboBoxLabel;
    /** Holds the MAC address policy check-box instance. */
    QIComboBox              *m_pMACComboBox;
    /** Holds the additional label instance. */
    QLabel                  *m_pAdditionalLabel;
    /** Holds the manifest check-box instance. */
    QCheckBox               *m_pManifestCheckbox;
    /** Holds the include ISOs check-box instance. */
    QCheckBox               *m_pIncludeISOsCheckbox;

    /** Holds the settings layout 2. */
    QGridLayout   *m_pSettingsLayout2;
    /** Holds the profile label instance. */
    QLabel        *m_pProfileLabel;
    /** Holds the profile combo-box instance. */
    QIComboBox    *m_pProfileComboBox;
    /** Holds the profile management tool-button instance. */
    QIToolButton  *m_pProfileToolButton;

    /** Holds the export mode label instance. */
    QLabel                                  *m_pExportModeLabel;
    /** Holds the export mode button group instance. */
    QButtonGroup                            *m_pExportModeButtonGroup;
    /** Holds the map of export mode button instances. */
    QMap<CloudExportMode, QAbstractButton*>  m_exportModeButtons;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageExpert_h */
