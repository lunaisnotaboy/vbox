/* $Id$ */
/** @file
 * VBox Qt GUI - UIMotherboardFeaturesEditor class implementation.
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

/* Qt includes: */
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>

/* GUI includes: */
#include "UIMotherboardFeaturesEditor.h"


UIMotherboardFeaturesEditor::UIMotherboardFeaturesEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_fEnableIoApic(false)
    , m_fEnableEfi(false)
    , m_fEnableUtcTime(false)
    , m_pLabel(0)
    , m_pCheckBoxEnableIoApic(0)
    , m_pCheckBoxEnableEfi(0)
    , m_pCheckBoxEnableUtcTime(0)
{
    prepare();
}

void UIMotherboardFeaturesEditor::setEnableIoApic(bool fOn)
{
    /* Update cached value and
     * check-box if value has changed: */
    if (m_fEnableIoApic != fOn)
    {
        m_fEnableIoApic = fOn;
        if (m_pCheckBoxEnableIoApic)
            m_pCheckBoxEnableIoApic->setCheckState(m_fEnableIoApic ? Qt::Checked : Qt::Unchecked);
    }
}

bool UIMotherboardFeaturesEditor::isEnabledIoApic() const
{
    return   m_pCheckBoxEnableIoApic
           ? m_pCheckBoxEnableIoApic->checkState() == Qt::Checked
           : m_fEnableIoApic;
}

void UIMotherboardFeaturesEditor::setEnableEfi(bool fOn)
{
    /* Update cached value and
     * check-box if value has changed: */
    if (m_fEnableEfi != fOn)
    {
        m_fEnableEfi = fOn;
        if (m_pCheckBoxEnableEfi)
            m_pCheckBoxEnableEfi->setCheckState(m_fEnableEfi ? Qt::Checked : Qt::Unchecked);
    }
}

bool UIMotherboardFeaturesEditor::isEnabledEfi() const
{
    return   m_pCheckBoxEnableEfi
           ? m_pCheckBoxEnableEfi->checkState() == Qt::Checked
           : m_fEnableEfi;
}

void UIMotherboardFeaturesEditor::setEnableUtcTime(bool fOn)
{
    /* Update cached value and
     * check-box if value has changed: */
    if (m_fEnableUtcTime != fOn)
    {
        m_fEnableUtcTime = fOn;
        if (m_pCheckBoxEnableUtcTime)
            m_pCheckBoxEnableUtcTime->setCheckState(m_fEnableUtcTime ? Qt::Checked : Qt::Unchecked);
    }
}

bool UIMotherboardFeaturesEditor::isEnabledUtcTime() const
{
    return   m_pCheckBoxEnableUtcTime
           ? m_pCheckBoxEnableUtcTime->checkState() == Qt::Checked
           : m_fEnableUtcTime;
}

int UIMotherboardFeaturesEditor::minimumLabelHorizontalHint() const
{
    return m_pLabel ? m_pLabel->minimumSizeHint().width() : 0;
}

void UIMotherboardFeaturesEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayout)
        m_pLayout->setColumnMinimumWidth(0, iIndent);
}

void UIMotherboardFeaturesEditor::retranslateUi()
{
    if (m_pLabel)
        m_pLabel->setText(tr("Extended Features:"));
    if (m_pCheckBoxEnableIoApic)
    {
        m_pCheckBoxEnableIoApic->setText(tr("Enable &I/O APIC"));
        m_pCheckBoxEnableIoApic->setToolTip(tr("When checked, the virtual machine will support the Input Output APIC (I/O APIC), "
                                               "which may slightly decrease performance. <b>Note:</b> don't disable this feature "
                                               "after having installed a Windows guest operating system!"));
    }
    if (m_pCheckBoxEnableEfi)
    {
        m_pCheckBoxEnableEfi->setText(tr("Enable &EFI (special OSes only)"));
        m_pCheckBoxEnableEfi->setToolTip(tr("When checked, the guest will support the Extended Firmware Interface (EFI), "
                                            "which is required to boot certain guest OSes. Non-EFI aware OSes will not be able "
                                            "to boot if this option is activated."));
    }
    if (m_pCheckBoxEnableUtcTime)
    {
        m_pCheckBoxEnableUtcTime->setText(tr("Hardware Clock in &UTC Time"));
        m_pCheckBoxEnableUtcTime->setToolTip(tr("When checked, the RTC device will report the time in UTC, otherwise in local "
                                                "(host) time. Unix usually expects the hardware clock to be set to UTC."));
    }
}

void UIMotherboardFeaturesEditor::prepare()
{
    /* Prepare main layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        m_pLayout->setContentsMargins(0, 0, 0, 0);
        m_pLayout->setColumnStretch(1, 1);

        /* Prepare label: */
        m_pLabel = new QLabel(this);
        if (m_pLabel)
        {
            m_pLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabel, 0, 0);
        }
        /* Prepare 'enable IO APIC' check-box: */
        m_pCheckBoxEnableIoApic = new QCheckBox(this);
        if (m_pCheckBoxEnableIoApic)
        {
            connect(m_pCheckBoxEnableIoApic, &QCheckBox::stateChanged,
                    this, &UIMotherboardFeaturesEditor::sigChangedIoApic);
            m_pLayout->addWidget(m_pCheckBoxEnableIoApic, 0, 1);
        }
        /* Prepare 'enable EFI' check-box: */
        m_pCheckBoxEnableEfi = new QCheckBox(this);
        if (m_pCheckBoxEnableEfi)
        {
            connect(m_pCheckBoxEnableEfi, &QCheckBox::stateChanged,
                    this, &UIMotherboardFeaturesEditor::sigChangedEfi);
            m_pLayout->addWidget(m_pCheckBoxEnableEfi, 1, 1);
        }
        /* Prepare 'enable UTC time' check-box: */
        m_pCheckBoxEnableUtcTime = new QCheckBox(this);
        if (m_pCheckBoxEnableUtcTime)
        {
            connect(m_pCheckBoxEnableUtcTime, &QCheckBox::stateChanged,
                    this, &UIMotherboardFeaturesEditor::sigChangedUtcTime);
            m_pLayout->addWidget(m_pCheckBoxEnableUtcTime, 2, 1);
        }
    }

    /* Apply language settings: */
    retranslateUi();
}
