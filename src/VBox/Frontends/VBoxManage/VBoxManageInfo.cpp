/* $Id$ */
/** @file
 * VBoxManage - VirtualBox's command-line interface.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef VBOX_ONLY_DOCS

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/EventQueue.h>

#include <VBox/com/VirtualBox.h>

#include <stdlib.h>
#include <stdarg.h>

#include <vector>
#include <list>

#include <iprt/runtime.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/asm.h>
#include <iprt/uuid.h>
#include <iprt/thread.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/env.h>
#include <iprt/cidr.h>
#include <VBox/err.h>
#include <VBox/version.h>
#include <VBox/VBoxHDD.h>

#include "VBoxManage.h"
using namespace com;


// funcs
///////////////////////////////////////////////////////////////////////////////



void showSnapshots(ComPtr<ISnapshot> rootSnapshot, VMINFO_DETAILS details, const Bstr &prefix /* = ""*/, int level /*= 0*/)
{
    /* start with the root */
    Bstr name;
    Guid uuid;
    rootSnapshot->COMGETTER(Name)(name.asOutParam());
    rootSnapshot->COMGETTER(Id)(uuid.asOutParam());
    if (details == VMINFO_MACHINEREADABLE)
    {
        /* print with hierarchical numbering */
        RTPrintf("SnapshotName%lS=\"%lS\"\n", prefix.raw(), name.raw());
        RTPrintf("SnapshotUUID%lS=\"%s\"\n", prefix.raw(), uuid.toString().raw());
    }
    else
    {
        /* print with indentation */
        RTPrintf("   %lSName: %lS (UUID: %s)\n", prefix.raw(), name.raw(), uuid.toString().raw());
    }

    /* get the children */
    ComPtr<ISnapshotCollection> coll;
    rootSnapshot->COMGETTER(Children)(coll.asOutParam());
    if (coll)
    {
        ComPtr<ISnapshotEnumerator> enumerator;
        coll->Enumerate(enumerator.asOutParam());
        ULONG index = 0;
        BOOL hasMore = FALSE;
        while (enumerator->HasMore(&hasMore), hasMore)
        {
            ComPtr<ISnapshot> snapshot;
            enumerator->GetNext(snapshot.asOutParam());
            if (snapshot)
            {
                Bstr newPrefix;
                if (details == VMINFO_MACHINEREADABLE)
                    newPrefix = Utf8StrFmt("%lS-%d", prefix.raw(), index + 1);
                else
                    newPrefix = Utf8StrFmt("%lS   ", prefix.raw());
                /* recursive call */
                showSnapshots(snapshot, details, newPrefix, level + 1);
            }
            index++;
        }
    }
}

static void makeTimeStr (char *s, int cb, int64_t millies)
{
    RTTIME t;
    RTTIMESPEC ts;

    RTTimeSpecSetMilli(&ts, millies);

    RTTimeExplode (&t, &ts);

    RTStrPrintf(s, cb, "%04d/%02d/%02d %02d:%02d:%02d UTC",
                        t.i32Year, t.u8Month, t.u8MonthDay,
                        t.u8Hour, t.u8Minute, t.u8Second);
}

HRESULT showVMInfo (ComPtr <IVirtualBox> virtualBox, ComPtr<IMachine> machine,
                    ComPtr <IConsole> console /*= ComPtr <IConsole> ()*/,
                    VMINFO_DETAILS details /*= VMINFO_NONE*/)
{
    HRESULT rc;

    /*
     * The rules for output in -argdump format:
     * 1) the key part (the [0-9a-zA-Z_]+ string before the '=' delimiter)
     *    is all lowercase for "VBoxManage modifyvm" parameters. Any
     *    other values printed are in CamelCase.
     * 2) strings (anything non-decimal) are printed surrounded by
     *    double quotes '"'. If the strings themselves contain double
     *    quotes, these characters are escaped by '\'. Any '\' character
     *    in the original string is also escaped by '\'.
     * 3) numbers (containing just [0-9\-]) are written out unchanged.
     */

    /** @todo the quoting is not yet implemented! */

    BOOL accessible = FALSE;
    CHECK_ERROR (machine, COMGETTER(Accessible) (&accessible));
    CheckComRCReturnRC (rc);

    if (!accessible)
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("name=\"<inaccessible>\"\n");
        else
            RTPrintf ("Name:            <inaccessible!>\n");
        Guid uuid;
        rc = machine->COMGETTER(Id) (uuid.asOutParam());
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf ("UUID=\"%s\"\n", uuid.toString().raw());
        else
            RTPrintf ("UUID:            %s\n", uuid.toString().raw());
        if (details != VMINFO_MACHINEREADABLE)
        {
            Bstr settingsFilePath;
            rc = machine->COMGETTER(SettingsFilePath) (settingsFilePath.asOutParam());
            RTPrintf ("Config file:     %lS\n", settingsFilePath.raw());
            ComPtr<IVirtualBoxErrorInfo> accessError;
            rc = machine->COMGETTER(AccessError) (accessError.asOutParam());
            RTPrintf ("Access error details:\n");
            ErrorInfo ei (accessError);
            PRINT_ERROR_INFO (ei);
            RTPrintf ("\n");
        }
        return S_OK;
    }

    Bstr machineName;
    rc = machine->COMGETTER(Name)(machineName.asOutParam());
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("name=\"%lS\"\n", machineName.raw());
    else
        RTPrintf("Name:            %lS\n", machineName.raw());

    Bstr osTypeId;
    rc = machine->COMGETTER(OSTypeId)(osTypeId.asOutParam());
    ComPtr<IGuestOSType> osType;
    rc = virtualBox->GetGuestOSType (osTypeId, osType.asOutParam());
    Bstr osName;
    rc = osType->COMGETTER(Description)(osName.asOutParam());
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("ostype=\"%lS\"\n", osTypeId.raw());
    else
        RTPrintf("Guest OS:        %lS\n", osName.raw());

    Guid uuid;
    rc = machine->COMGETTER(Id)(uuid.asOutParam());
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("UUID=\"%s\"\n", uuid.toString().raw());
    else
        RTPrintf("UUID:            %s\n", uuid.toString().raw());

    Bstr settingsFilePath;
    rc = machine->COMGETTER(SettingsFilePath)(settingsFilePath.asOutParam());
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("CfgFile=\"%lS\"\n", settingsFilePath.raw());
    else
        RTPrintf("Config file:     %lS\n", settingsFilePath.raw());

    ULONG memorySize;
    rc = machine->COMGETTER(MemorySize)(&memorySize);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("memory=%u\n", memorySize);
    else
        RTPrintf("Memory size:     %uMB\n", memorySize);

    ULONG vramSize;
    rc = machine->COMGETTER(VRAMSize)(&vramSize);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("vram=%u\n", vramSize);
    else
        RTPrintf("VRAM size:       %uMB\n", vramSize);

    ComPtr <IBIOSSettings> biosSettings;
    machine->COMGETTER(BIOSSettings)(biosSettings.asOutParam());

    BIOSBootMenuMode_T bootMenuMode;
    biosSettings->COMGETTER(BootMenuMode)(&bootMenuMode);
    const char *pszBootMenu = NULL;
    switch (bootMenuMode)
    {
        case BIOSBootMenuMode_Disabled:
            pszBootMenu = "disabled";
            break;
        case BIOSBootMenuMode_MenuOnly:
            if (details == VMINFO_MACHINEREADABLE)
                pszBootMenu = "menuonly";
            else
                pszBootMenu = "menu only";
            break;
        default:
            if (details == VMINFO_MACHINEREADABLE)
                pszBootMenu = "messageandmenu";
            else
                pszBootMenu = "message and menu";
    }
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("bootmenu=\"%s\"\n", pszBootMenu);
    else
        RTPrintf("Boot menu mode:  %s\n", pszBootMenu);

    BOOL acpiEnabled;
    biosSettings->COMGETTER(ACPIEnabled)(&acpiEnabled);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("acpi=\"%s\"\n", acpiEnabled ? "on" : "off");
    else
        RTPrintf("ACPI:            %s\n", acpiEnabled ? "on" : "off");

    BOOL ioapicEnabled;
    biosSettings->COMGETTER(IOAPICEnabled)(&ioapicEnabled);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("ioapic=\"%s\"\n", ioapicEnabled ? "on" : "off");
    else
        RTPrintf("IOAPIC:          %s\n", ioapicEnabled ? "on" : "off");

    BOOL PAEEnabled;
    machine->COMGETTER(PAEEnabled)(&PAEEnabled);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("pae=\"%s\"\n", PAEEnabled ? "on" : "off");
    else
        RTPrintf("PAE:             %s\n", PAEEnabled ? "on" : "off");

    LONG64 timeOffset;
    biosSettings->COMGETTER(TimeOffset)(&timeOffset);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("biossystemtimeoffset=%lld\n", timeOffset);
    else
        RTPrintf("Time offset:     %lld ms\n", timeOffset);

    TSBool_T hwVirtExEnabled;
    machine->COMGETTER(HWVirtExEnabled)(&hwVirtExEnabled);
    if (hwVirtExEnabled == TSBool_Default)
    {
        BOOL fHWVirtExEnabled;
        ComPtr<ISystemProperties> systemProperties;
        virtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());
        systemProperties->COMGETTER(HWVirtExEnabled)(&fHWVirtExEnabled);
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("hwvirtex=\"default\"\n");
        else
            RTPrintf("Hardw. virt.ext: Default (%s)\n", fHWVirtExEnabled ? "on" : "off");
    }
    else
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("hwvirtex=\"%s\"\n", hwVirtExEnabled == TSBool_True ? "on" : "off");
        else
            RTPrintf("Hardw. virt.ext: %s\n", hwVirtExEnabled == TSBool_True ? "on" : "off");
    }
    BOOL HWVirtExNestedPagingEnabled;
    machine->COMGETTER(HWVirtExNestedPagingEnabled)(&HWVirtExNestedPagingEnabled);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("nestedpaging=\"%s\"\n", HWVirtExNestedPagingEnabled ? "on" : "off");
    else
        RTPrintf("Nested Paging:   %s\n", HWVirtExNestedPagingEnabled ? "on" : "off");

    BOOL HWVirtExVPIDEnabled;
    machine->COMGETTER(HWVirtExVPIDEnabled)(&HWVirtExVPIDEnabled);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("vtxvpid=\"%s\"\n", HWVirtExVPIDEnabled ? "on" : "off");
    else
        RTPrintf("VT-x VPID:       %s\n", HWVirtExVPIDEnabled ? "on" : "off");

    MachineState_T machineState;
    const char *pszState = NULL;
    rc = machine->COMGETTER(State)(&machineState);
    switch (machineState)
    {
        case MachineState_PoweredOff:
            if (details == VMINFO_MACHINEREADABLE)
                pszState = "poweroff";
            else
                pszState = "powered off";
            break;
        case MachineState_Saved:
            pszState = "saved";
            break;
        case MachineState_Aborted:
            pszState = "aborted";
            break;
        case MachineState_Running:
            pszState = "running";
            break;
        case MachineState_Paused:
            pszState = "paused";
            break;
        case MachineState_Starting:
            pszState = "starting";
            break;
        case MachineState_Stopping:
            pszState = "stopping";
            break;
        case MachineState_Saving:
            pszState = "saving";
            break;
        case MachineState_Restoring:
            pszState = "restoring";
            break;
        default:
            pszState = "unknown";
            break;
    }
    LONG64 stateSince;
    machine->COMGETTER(LastStateChange)(&stateSince);
    RTTIMESPEC timeSpec;
    RTTimeSpecSetMilli(&timeSpec, stateSince);
    char pszTime[30] = {0};
    RTTimeSpecToString(&timeSpec, pszTime, sizeof(pszTime));
    if (details == VMINFO_MACHINEREADABLE)
    {
        RTPrintf("VMState=\"%s\"\n", pszState);
        RTPrintf("VMStateChangeTime=\"%s\"\n", pszTime);
    }
    else
        RTPrintf("State:           %s (since %s)\n", pszState, pszTime);

    ULONG numMonitors;
    machine->COMGETTER(MonitorCount)(&numMonitors);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("monitorcount=%d\n", numMonitors);
    else
        RTPrintf("Monitor count:   %d\n", numMonitors);

    BOOL accelerate3d;
    machine->COMGETTER(Accelerate3DEnabled)(&accelerate3d);
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("accelerate3d=\"%s\"\n", accelerate3d ? "on" : "off");
    else
        RTPrintf("3D Acceleration:       %s\n", accelerate3d ? "on" : "off");

    ComPtr<IFloppyDrive> floppyDrive;
    rc = machine->COMGETTER(FloppyDrive)(floppyDrive.asOutParam());
    if (SUCCEEDED(rc) && floppyDrive)
    {
        BOOL fFloppyEnabled;
        floppyDrive->COMGETTER(Enabled)(&fFloppyEnabled);
        Utf8Str pszFloppy = "invalid";
        if (fFloppyEnabled)
        {
            DriveState_T floppyState;
            floppyDrive->COMGETTER(State)(&floppyState);
            switch (floppyState)
            {
                case DriveState_ImageMounted:
                {
                    ComPtr<IFloppyImage2> floppyImage;
                    rc = floppyDrive->GetImage(floppyImage.asOutParam());
                    if (SUCCEEDED(rc) && floppyImage)
                    {
                        Bstr imagePath;
                        floppyImage->COMGETTER(Location)(imagePath.asOutParam());
                        Guid imageGuid;
                        floppyImage->COMGETTER(Id)(imageGuid.asOutParam());
                        if (details == VMINFO_MACHINEREADABLE)
                        {
                            RTPrintf("FloppyImageUUID=\"%s\"\n", imageGuid.toString().raw());
                            pszFloppy = Utf8StrFmt("%lS", imagePath.raw());
                        }
                        else
                            pszFloppy = Utf8StrFmt("%lS (UUID: %s)", imagePath.raw(), imageGuid.toString().raw());
                    }
                    break;
                }

                case DriveState_HostDriveCaptured:
                {
                    ComPtr<IHostFloppyDrive> hostFloppyDrive;
                    rc = floppyDrive->GetHostDrive(hostFloppyDrive.asOutParam());
                    if (SUCCEEDED(rc) && floppyDrive)
                    {
                        Bstr driveName;
                        hostFloppyDrive->COMGETTER(Name)(driveName.asOutParam());
                        if (details == VMINFO_MACHINEREADABLE)
                            pszFloppy = Utf8StrFmt("host:%lS", driveName.raw());
                        else
                            pszFloppy = Utf8StrFmt("Host drive %lS", driveName.raw());
                    }
                    break;
                }

                case DriveState_NotMounted:
                {
                    pszFloppy = "empty";
                    break;
                }
            }
        }
        else
        {
            pszFloppy = "disabled";
        }
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("floppy=\"%s\"\n", pszFloppy.raw());
        else
            RTPrintf("Floppy:          %s\n", pszFloppy.raw());
    }

    /*
     * SATA.
     *
     * Contributed by: James Lucas
     */
#ifdef VBOX_WITH_AHCI
    ComPtr<ISATAController> SATACtl;
    BOOL fSataEnabled;
    rc = machine->COMGETTER(SATAController)(SATACtl.asOutParam());
    if (SUCCEEDED(rc))
    {
        rc = SATACtl->COMGETTER(Enabled)(&fSataEnabled);
        if (FAILED(rc))
            fSataEnabled = false;
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("sata=\"%s\"\n", fSataEnabled ? "on" : "off");
        else
            RTPrintf("SATA:            %s\n", fSataEnabled ? "enabled" : "disabled");
    }

    /*
     * SATA Hard disks
     */
    if (fSataEnabled)
    {
        ComPtr<IHardDisk2> hardDisk;
        Bstr  filePath;
        ULONG cSataPorts;

        SATACtl->COMGETTER(PortCount)(&cSataPorts);
        for (ULONG i = 0; i < cSataPorts; ++ i)
        {
            rc = machine->GetHardDisk2(StorageBus_SATA, i, 0, hardDisk.asOutParam());
            if (SUCCEEDED(rc) && hardDisk)
            {
                hardDisk->COMGETTER(Location)(filePath.asOutParam());
                hardDisk->COMGETTER(Id)(uuid.asOutParam());
                if (details == VMINFO_MACHINEREADABLE)
                {
                    RTPrintf("sata%d=\"%lS\"\n", i, filePath.raw());
                    RTPrintf("sata%dImageUUID=\"%s\"\n", i, uuid.toString().raw());
                }
                else
                    RTPrintf("SATA %d:          %lS (UUID: %s)\n", i, filePath.raw(), uuid.toString().raw());
            }
            else
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("sata%d=\"none\"\n",i);
            }
        }
    }
#endif

    /*
     * IDE Hard disks
     */
    IDEControllerType_T ideController;
    const char *pszIdeController = NULL;
    biosSettings->COMGETTER(IDEControllerType)(&ideController);
    switch (ideController)
    {
        case IDEControllerType_PIIX3:
            pszIdeController = "PIIX3";
            break;
        case IDEControllerType_PIIX4:
            pszIdeController = "PIIX4";
            break;
        default:
            pszIdeController = "unknown";
    }
    if (details == VMINFO_MACHINEREADABLE)
        RTPrintf("idecontroller=\"%s\"\n", pszIdeController);
    else
        RTPrintf("IDE Controller:  %s\n", pszIdeController);

    ComPtr<IHardDisk2> hardDisk;
    Bstr filePath;
    rc = machine->GetHardDisk2(StorageBus_IDE, 0, 0, hardDisk.asOutParam());
    if (SUCCEEDED(rc) && hardDisk)
    {
        hardDisk->COMGETTER(Location)(filePath.asOutParam());
        hardDisk->COMGETTER(Id)(uuid.asOutParam());
        if (details == VMINFO_MACHINEREADABLE)
        {
            RTPrintf("hda=\"%lS\"\n", filePath.raw());
            RTPrintf("HdaImageUUID=\"%s\"\n", uuid.toString().raw());
        }
        else
            RTPrintf("Primary master:  %lS (UUID: %s)\n", filePath.raw(), uuid.toString().raw());
    }
    else
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("hda=\"none\"\n");
    }
    rc = machine->GetHardDisk2(StorageBus_IDE, 0, 1, hardDisk.asOutParam());
    if (SUCCEEDED(rc) && hardDisk)
    {
        hardDisk->COMGETTER(Location)(filePath.asOutParam());
        hardDisk->COMGETTER(Id)(uuid.asOutParam());
        if (details == VMINFO_MACHINEREADABLE)
        {
            RTPrintf("hdb=\"%lS\"\n", filePath.raw());
            RTPrintf("HdbImageUUID=\"%s\"\n", uuid.toString().raw());
        }
        else
            RTPrintf("Primary slave:   %lS (UUID: %s)\n", filePath.raw(), uuid.toString().raw());
    }
    else
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("hdb=\"none\"\n");
    }
    rc = machine->GetHardDisk2(StorageBus_IDE, 1, 1, hardDisk.asOutParam());
    if (SUCCEEDED(rc) && hardDisk)
    {
        hardDisk->COMGETTER(Location)(filePath.asOutParam());
        hardDisk->COMGETTER(Id)(uuid.asOutParam());
        if (details == VMINFO_MACHINEREADABLE)
        {
            RTPrintf("hdd=\"%lS\"\n", filePath.raw());
            RTPrintf("HddImageUUID=\"%s\"\n", uuid.toString().raw());
        }
        else
            RTPrintf("Secondary slave: %lS (UUID: %s)\n", filePath.raw(), uuid.toString().raw());
    }
    else
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("hdd=\"none\"\n");
    }
    ComPtr<IDVDDrive> dvdDrive;
    rc = machine->COMGETTER(DVDDrive)(dvdDrive.asOutParam());
    if (SUCCEEDED(rc) && dvdDrive)
    {
        ComPtr<IDVDImage2> dvdImage;
        rc = dvdDrive->GetImage(dvdImage.asOutParam());
        if (SUCCEEDED(rc) && dvdImage)
        {
            rc = dvdImage->COMGETTER(Location)(filePath.asOutParam());
            if (SUCCEEDED(rc) && filePath)
            {
                rc = dvdImage->COMGETTER(Id)(uuid.asOutParam());
                if (details == VMINFO_MACHINEREADABLE)
                {
                    RTPrintf("dvd=\"%lS\"\n", filePath.raw());
                    RTPrintf("DvdImageUUID=\"%s\"\n", uuid.toString().raw());
                }
                else
                    RTPrintf("DVD:             %lS (UUID: %s)\n", filePath.raw(), uuid.toString().raw());
            }
        }
        else
        {
            ComPtr<IHostDVDDrive> hostDVDDrive;
            rc = dvdDrive->GetHostDrive(hostDVDDrive.asOutParam());
            if (SUCCEEDED(rc) && hostDVDDrive)
            {
                Bstr name;
                hostDVDDrive->COMGETTER(Name)(name.asOutParam());
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("dvd=\"host:%lS\"\n", name.raw());
                else
                    RTPrintf("DVD:             Host drive %lS", name.raw());
            }
            else
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("dvd=\"none\"\n");
                else
                    RTPrintf("DVD:             empty");
            }
            BOOL fPassthrough;
            dvdDrive->COMGETTER(Passthrough)(&fPassthrough);
            if (details == VMINFO_MACHINEREADABLE)
            {
                RTPrintf("dvdpassthrough=\"%s\"\n", fPassthrough ? "on" : "off");
            }
            else
            {
                if (fPassthrough)
                    RTPrintf(" (passthrough enabled)");
                RTPrintf("\n");
            }
        }
    }

    /* get the maximum amount of NICS */
    ComPtr<ISystemProperties> sysProps;
    virtualBox->COMGETTER(SystemProperties)(sysProps.asOutParam());
    ULONG maxNICs = 0;
    sysProps->COMGETTER(NetworkAdapterCount)(&maxNICs);
    for (ULONG currentNIC = 0; currentNIC < maxNICs; currentNIC++)
    {
        ComPtr<INetworkAdapter> nic;
        rc = machine->GetNetworkAdapter(currentNIC, nic.asOutParam());
        if (SUCCEEDED(rc) && nic)
        {
            BOOL fEnabled;
            nic->COMGETTER(Enabled)(&fEnabled);
            if (!fEnabled)
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("nic%d=\"none\"\n", currentNIC + 1);
                else
                    RTPrintf("NIC %d:           disabled\n", currentNIC + 1);
            }
            else
            {
                Bstr strMACAddress;
                nic->COMGETTER(MACAddress)(strMACAddress.asOutParam());
                Utf8Str strAttachment;
                NetworkAttachmentType_T attachment;
                nic->COMGETTER(AttachmentType)(&attachment);
                switch (attachment)
                {
                    case NetworkAttachmentType_Null:
                        if (details == VMINFO_MACHINEREADABLE)
                            strAttachment = "null";
                        else
                            strAttachment = "none";
                        break;
                    case NetworkAttachmentType_NAT:
                    {
                        Bstr strNetwork;
                        nic->COMGETTER(NATNetwork)(strNetwork.asOutParam());
                        if (details == VMINFO_MACHINEREADABLE)
                        {
                            RTPrintf("natnet%d=\"%lS\"\n", currentNIC + 1, strNetwork.raw());
                            strAttachment = "nat";
                        }
                        else if (!strNetwork.isEmpty())
                            strAttachment = Utf8StrFmt("NAT (%lS)", strNetwork.raw());
                        else
                            strAttachment = "NAT";
                        break;
                    }
                    case NetworkAttachmentType_HostInterface:
                    {
                        Bstr strHostIfDev;
                        nic->COMGETTER(HostInterface)(strHostIfDev.asOutParam());
                        if (details == VMINFO_MACHINEREADABLE)
                        {
                            RTPrintf("hostifdev%d=\"%lS\"\n", currentNIC + 1, strHostIfDev.raw());
                            strAttachment = "hostif";
                        }
                        else
                            strAttachment = Utf8StrFmt("Host Interface '%lS'", strHostIfDev.raw());
                        break;
                    }
                    case NetworkAttachmentType_Internal:
                    {
                        Bstr strNetwork;
                        nic->COMGETTER(InternalNetwork)(strNetwork.asOutParam());
                        if (details == VMINFO_MACHINEREADABLE)
                        {
                            RTPrintf("intnet%d=\"%lS\"\n", currentNIC + 1, strNetwork.raw());
                            strAttachment = "intnet";
                        }
                        else
                            strAttachment = Utf8StrFmt("Internal Network '%s'", Utf8Str(strNetwork).raw());
                        break;
                    }
                    default:
                        strAttachment = "unknown";
                        break;
                }

                /* cable connected */
                BOOL fConnected;
                nic->COMGETTER(CableConnected)(&fConnected);

                /* trace stuff */
                BOOL fTraceEnabled;
                nic->COMGETTER(TraceEnabled)(&fTraceEnabled);
                Bstr traceFile;
                nic->COMGETTER(TraceFile)(traceFile.asOutParam());

                /* NIC type */
                Utf8Str strNICType;
                NetworkAdapterType_T NICType;
                nic->COMGETTER(AdapterType)(&NICType);
                switch (NICType) {
                case NetworkAdapterType_Am79C970A:
                    strNICType = "Am79C970A";
                    break;
                case NetworkAdapterType_Am79C973:
                    strNICType = "Am79C973";
                    break;
#ifdef VBOX_WITH_E1000
                case NetworkAdapterType_I82540EM:
                    strNICType = "82540EM";
                    break;
                case NetworkAdapterType_I82543GC:
                    strNICType = "82543GC";
                    break;
#endif
                default:
                    strNICType = "unknown";
                    break;
                }

                /* reported line speed */
                ULONG ulLineSpeed;
                nic->COMGETTER(LineSpeed)(&ulLineSpeed);

                if (details == VMINFO_MACHINEREADABLE)
                {
                    RTPrintf("macaddress%d=\"%lS\"\n", currentNIC + 1, strMACAddress.raw());
                    RTPrintf("cableconnected%d=\"%s\"\n", currentNIC + 1, fConnected ? "on" : "off");
                    RTPrintf("nic%d=\"%s\"\n", currentNIC + 1, strAttachment.raw());
                }
                else
                    RTPrintf("NIC %d:           MAC: %lS, Attachment: %s, Cable connected: %s, Trace: %s (file: %lS), Type: %s, Reported speed: %d Mbps\n",
                             currentNIC + 1, strMACAddress.raw(), strAttachment.raw(),
                             fConnected ? "on" : "off",
                             fTraceEnabled ? "on" : "off",
                             traceFile.isEmpty() ? Bstr("none").raw() : traceFile.raw(),
                             strNICType.raw(),
                             ulLineSpeed / 1000);
            }
        }
    }

    /* get the maximum amount of UARTs */
    ULONG maxUARTs = 0;
    sysProps->COMGETTER(SerialPortCount)(&maxUARTs);
    for (ULONG currentUART = 0; currentUART < maxUARTs; currentUART++)
    {
        ComPtr<ISerialPort> uart;
        rc = machine->GetSerialPort(currentUART, uart.asOutParam());
        if (SUCCEEDED(rc) && uart)
        {
            BOOL fEnabled;
            uart->COMGETTER(Enabled)(&fEnabled);
            if (!fEnabled)
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("uart%d=\"off\"\n", currentUART + 1);
                else
                    RTPrintf("UART %d:          disabled\n", currentUART + 1);
            }
            else
            {
                ULONG ulIRQ, ulIOBase;
                PortMode_T HostMode;
                Bstr path;
                BOOL fServer;
                uart->COMGETTER(IRQ)(&ulIRQ);
                uart->COMGETTER(IOBase)(&ulIOBase);
                uart->COMGETTER(Path)(path.asOutParam());
                uart->COMGETTER(Server)(&fServer);
                uart->COMGETTER(HostMode)(&HostMode);

                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("uart%d=\"%#06x,%d\"\n", currentUART + 1,
                             ulIOBase, ulIRQ);
                else
                    RTPrintf("UART %d:          I/O base: 0x%04x, IRQ: %d",
                             currentUART + 1, ulIOBase, ulIRQ);
                switch (HostMode)
                {
                    default:
                    case PortMode_Disconnected:
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("uartmode%d=\"disconnected\"\n", currentUART + 1);
                        else
                            RTPrintf(", disconnected\n");
                        break;
                    case PortMode_HostPipe:
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("uartmode%d=\"%s,%lS\"\n", currentUART + 1,
                                     fServer ? "server" : "client", path.raw());
                        else
                            RTPrintf(", attached to pipe (%s) '%lS'\n",
                                     fServer ? "server" : "client", path.raw());
                        break;
                    case PortMode_HostDevice:
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("uartmode%d=\"%lS\"\n", currentUART + 1,
                                     path.raw());
                        else
                            RTPrintf(", attached to device '%lS'\n", path.raw());
                        break;
                }
            }
        }
    }

    ComPtr<IAudioAdapter> AudioAdapter;
    rc = machine->COMGETTER(AudioAdapter)(AudioAdapter.asOutParam());
    if (SUCCEEDED(rc))
    {
        const char *pszDrv  = "Unknown";
        const char *pszCtrl = "Unknown";
        BOOL fEnabled;
        rc = AudioAdapter->COMGETTER(Enabled)(&fEnabled);
        if (SUCCEEDED(rc) && fEnabled)
        {
            AudioDriverType_T enmDrvType;
            rc = AudioAdapter->COMGETTER(AudioDriver)(&enmDrvType);
            switch (enmDrvType)
            {
                case AudioDriverType_Null:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "null";
                    else
                        pszDrv = "Null";
                    break;
                case AudioDriverType_WinMM:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "winmm";
                    else
                        pszDrv = "WINMM";
                    break;
                case AudioDriverType_DirectSound:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "dsound";
                    else
                        pszDrv = "DSOUND";
                    break;
                case AudioDriverType_OSS:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "oss";
                    else
                        pszDrv = "OSS";
                    break;
                case AudioDriverType_ALSA:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "alsa";
                    else
                        pszDrv = "ALSA";
                    break;
                case AudioDriverType_Pulse:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "pulse";
                    else
                        pszDrv = "PulseAudio";
                    break;
                case AudioDriverType_CoreAudio:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "coreaudio";
                    else
                        pszDrv = "CoreAudio";
                    break;
                case AudioDriverType_SolAudio:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "solaudio";
                    else
                        pszDrv = "SolAudio";
                    break;
                default:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszDrv = "unknown";
                    break;
            }
            AudioControllerType_T enmCtrlType;
            rc = AudioAdapter->COMGETTER(AudioController)(&enmCtrlType);
            switch (enmCtrlType)
            {
                case AudioControllerType_AC97:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszCtrl = "ac97";
                    else
                        pszCtrl = "AC97";
                    break;
                case AudioControllerType_SB16:
                    if (details == VMINFO_MACHINEREADABLE)
                        pszCtrl = "sb16";
                    else
                        pszCtrl = "SB16";
                    break;
            }
        }
        else
            fEnabled = FALSE;
        if (details == VMINFO_MACHINEREADABLE)
        {
            if (fEnabled)
                RTPrintf("audio=\"%s\"\n", pszDrv);
            else
                RTPrintf("audio=\"none\"\n");
        }
        else
            RTPrintf("Audio:           %s (Driver: %s, Controller: %s)\n",
                    fEnabled ? "enabled" : "disabled", pszDrv, pszCtrl);
    }

    /* Shared clipboard */
    {
        const char *psz = "Unknown";
        ClipboardMode_T enmMode;
        rc = machine->COMGETTER(ClipboardMode)(&enmMode);
        switch (enmMode)
        {
            case ClipboardMode_Disabled:
                if (details == VMINFO_MACHINEREADABLE)
                    psz = "disabled";
                else
                    psz = "disabled";
                break;
            case ClipboardMode_HostToGuest:
                if (details == VMINFO_MACHINEREADABLE)
                    psz = "hosttoguest";
                else
                    psz = "HostToGuest";
                break;
            case ClipboardMode_GuestToHost:
                if (details == VMINFO_MACHINEREADABLE)
                    psz = "guesttohost";
                else
                    psz = "GuestToHost";
                break;
            case ClipboardMode_Bidirectional:
                if (details == VMINFO_MACHINEREADABLE)
                    psz = "bidirectional";
                else
                    psz = "Bidirectional";
                break;
            default:
                if (details == VMINFO_MACHINEREADABLE)
                    psz = "unknown";
                break;
        }
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("clipboard=\"%s\"\n", psz);
        else
            RTPrintf("Clipboard Mode:  %s\n", psz);
    }

    if (console)
    {
        ComPtr<IDisplay> display;
        CHECK_ERROR_RET(console, COMGETTER(Display)(display.asOutParam()), rc);
        do
        {
            ULONG xRes, yRes, bpp;
            rc = display->COMGETTER(Width)(&xRes);
            if (rc == E_ACCESSDENIED)
                break; /* VM not powered up */
            if (FAILED(rc))
            {
                com::ErrorInfo info (display);
                PRINT_ERROR_INFO (info);
                return rc;
            }
            rc = display->COMGETTER(Height)(&yRes);
            if (rc == E_ACCESSDENIED)
                break; /* VM not powered up */
            if (FAILED(rc))
            {
                com::ErrorInfo info (display);
                PRINT_ERROR_INFO (info);
                return rc;
            }
            rc = display->COMGETTER(BitsPerPixel)(&bpp);
            if (rc == E_ACCESSDENIED)
                break; /* VM not powered up */
            if (FAILED(rc))
            {
                com::ErrorInfo info (display);
                PRINT_ERROR_INFO (info);
                return rc;
            }
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("VideoMode=\"%d,%d,%d\"\n", xRes, yRes, bpp);
            else
                RTPrintf("Video mode:      %dx%dx%d\n", xRes, yRes, bpp);
        }
        while (0);
    }

    /*
     * VRDP
     */
    ComPtr<IVRDPServer> vrdpServer;
    rc = machine->COMGETTER(VRDPServer)(vrdpServer.asOutParam());
    if (SUCCEEDED(rc) && vrdpServer)
    {
        BOOL fEnabled = false;
        vrdpServer->COMGETTER(Enabled)(&fEnabled);
        if (fEnabled)
        {
            ULONG port;
            vrdpServer->COMGETTER(Port)(&port);
            Bstr address;
            vrdpServer->COMGETTER(NetAddress)(address.asOutParam());
            BOOL fMultiCon;
            vrdpServer->COMGETTER(AllowMultiConnection)(&fMultiCon);
            BOOL fReuseCon;
            vrdpServer->COMGETTER(ReuseSingleConnection)(&fReuseCon);
            VRDPAuthType_T vrdpAuthType;
            const char *strAuthType;
            vrdpServer->COMGETTER(AuthType)(&vrdpAuthType);
            switch (vrdpAuthType)
            {
                case VRDPAuthType_Null:
                    strAuthType = "null";
                    break;
                case VRDPAuthType_External:
                    strAuthType = "external";
                    break;
                case VRDPAuthType_Guest:
                    strAuthType = "guest";
                    break;
                default:
                    strAuthType = "unknown";
                    break;
            }
            if (details == VMINFO_MACHINEREADABLE)
            {
                RTPrintf("vrdp=\"on\"\n");
                RTPrintf("vrdpport=%d\n", port);
                RTPrintf("vrdpaddress=\"%lS\"\n", address.raw());
                RTPrintf("vrdpauthtype=\"%s\"\n", strAuthType);
                RTPrintf("vrdpmulticon=\"%s\"\n", fMultiCon ? "on" : "off");
                RTPrintf("vrdpreusecon=\"%s\"\n", fReuseCon ? "on" : "off");
            }
            else
            {
                if (address.isEmpty())
                    address = "0.0.0.0";
                RTPrintf("VRDP:            enabled (Address %lS, Port %d, MultiConn: %s, ReuseSingleConn: %s, Authentication type: %s)\n", address.raw(), port, fMultiCon ? "on" : "off", fReuseCon ? "on" : "off", strAuthType);
            }
        }
        else
        {
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("vrdp=\"off\"\n");
            else
                RTPrintf("VRDP:            disabled\n");
        }
    }

    /*
     * USB.
     */
    ComPtr<IUSBController> USBCtl;
    rc = machine->COMGETTER(USBController)(USBCtl.asOutParam());
    if (SUCCEEDED(rc))
    {
        BOOL fEnabled;
        rc = USBCtl->COMGETTER(Enabled)(&fEnabled);
        if (FAILED(rc))
            fEnabled = false;
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("usb=\"%s\"\n", fEnabled ? "on" : "off");
        else
            RTPrintf("USB:             %s\n", fEnabled ? "enabled" : "disabled");

        if (details != VMINFO_MACHINEREADABLE)
            RTPrintf("\nUSB Device Filters:\n\n");

        ComPtr<IUSBDeviceFilterCollection> Coll;
        CHECK_ERROR_RET (USBCtl, COMGETTER(DeviceFilters)(Coll.asOutParam()), rc);

        ComPtr<IUSBDeviceFilterEnumerator> Enum;
        CHECK_ERROR_RET (Coll, Enumerate(Enum.asOutParam()), rc);

        ULONG index = 0;
        BOOL fMore = FALSE;
        rc = Enum->HasMore (&fMore);
        ASSERT_RET (SUCCEEDED (rc), rc);

        if (!fMore)
        {
            if (details != VMINFO_MACHINEREADABLE)
                RTPrintf("<none>\n\n");
        }
        else
        while (fMore)
        {
            ComPtr<IUSBDeviceFilter> DevPtr;
            rc = Enum->GetNext(DevPtr.asOutParam());
            ASSERT_RET (SUCCEEDED (rc), rc);

            /* Query info. */

            if (details != VMINFO_MACHINEREADABLE)
                RTPrintf("Index:            %lu\n", index);

            BOOL bActive = FALSE;
            CHECK_ERROR_RET (DevPtr, COMGETTER (Active) (&bActive), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterActive%d=\"%s\"\n", index + 1, bActive ? "on" : "off");
            else
                RTPrintf("Active:           %s\n", bActive ? "yes" : "no");

            Bstr bstr;
            CHECK_ERROR_RET (DevPtr, COMGETTER (Name) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterName%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("Name:             %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (VendorId) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterVendorId%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("VendorId:         %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (ProductId) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterProductId%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("ProductId:        %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (Revision) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterRevision%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("Revision:         %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (Manufacturer) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterManufacturer%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("Manufacturer:     %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (Product) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterProduct%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("Product:          %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (Remote) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterRemote%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("Remote:           %lS\n", bstr.raw());
            CHECK_ERROR_RET (DevPtr, COMGETTER (SerialNumber) (bstr.asOutParam()), rc);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("USBFilterSerialNumber%d=\"%lS\"\n", index + 1, bstr.raw());
            else
                RTPrintf("Serial Number:    %lS\n", bstr.raw());
            if (details != VMINFO_MACHINEREADABLE)
            {
                ULONG fMaskedIfs;
                CHECK_ERROR_RET (DevPtr, COMGETTER (MaskedInterfaces) (&fMaskedIfs), rc);
                if (fMaskedIfs)
                    RTPrintf("Masked Interfaces: 0x%08x\n", fMaskedIfs);
                RTPrintf("\n");
            }

            rc = Enum->HasMore (&fMore);
            ASSERT_RET (SUCCEEDED (rc), rc);

            index ++;
        }

        if (console)
        {
            index = 0;
            /* scope */
            {
                if (details != VMINFO_MACHINEREADABLE)
                    RTPrintf("Available remote USB devices:\n\n");

                ComPtr<IHostUSBDeviceCollection> coll;
                CHECK_ERROR_RET (console, COMGETTER(RemoteUSBDevices) (coll.asOutParam()), rc);

                ComPtr <IHostUSBDeviceEnumerator> en;
                CHECK_ERROR_RET (coll, Enumerate (en.asOutParam()), rc);

                BOOL more = FALSE;
                rc = en->HasMore (&more);
                ASSERT_RET (SUCCEEDED (rc), rc);

                if (!more)
                {
                    if (details != VMINFO_MACHINEREADABLE)
                        RTPrintf("<none>\n\n");
                }
                else
                while (more)
                {
                    ComPtr <IHostUSBDevice> dev;
                    rc = en->GetNext (dev.asOutParam());
                    ASSERT_RET (SUCCEEDED (rc), rc);

                    /* Query info. */
                    Guid id;
                    CHECK_ERROR_RET (dev, COMGETTER(Id)(id.asOutParam()), rc);
                    USHORT usVendorId;
                    CHECK_ERROR_RET (dev, COMGETTER(VendorId)(&usVendorId), rc);
                    USHORT usProductId;
                    CHECK_ERROR_RET (dev, COMGETTER(ProductId)(&usProductId), rc);
                    USHORT bcdRevision;
                    CHECK_ERROR_RET (dev, COMGETTER(Revision)(&bcdRevision), rc);

                    if (details == VMINFO_MACHINEREADABLE)
                        RTPrintf("USBRemoteUUID%d=\"%S\"\n"
                                 "USBRemoteVendorId%d=\"%#06x\"\n"
                                 "USBRemoteProductId%d=\"%#06x\"\n"
                                 "USBRemoteRevision%d=\"%#04x%02x\"\n",
                                 index + 1, id.toString().raw(),
                                 index + 1, usVendorId,
                                 index + 1, usProductId,
                                 index + 1, bcdRevision >> 8, bcdRevision & 0xff);
                    else
                        RTPrintf("UUID:               %S\n"
                                 "VendorId:           0x%04x (%04X)\n"
                                 "ProductId:          0x%04x (%04X)\n"
                                 "Revision:           %u.%u (%02u%02u)\n",
                                 id.toString().raw(),
                                 usVendorId, usVendorId, usProductId, usProductId,
                                 bcdRevision >> 8, bcdRevision & 0xff,
                                 bcdRevision >> 8, bcdRevision & 0xff);

                    /* optional stuff. */
                    Bstr bstr;
                    CHECK_ERROR_RET (dev, COMGETTER(Manufacturer)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBRemoteManufacturer%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("Manufacturer:       %lS\n", bstr.raw());
                    }
                    CHECK_ERROR_RET (dev, COMGETTER(Product)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBRemoteProduct%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("Product:            %lS\n", bstr.raw());
                    }
                    CHECK_ERROR_RET (dev, COMGETTER(SerialNumber)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBRemoteSerialNumber%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("SerialNumber:       %lS\n", bstr.raw());
                    }
                    CHECK_ERROR_RET (dev, COMGETTER(Address)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBRemoteAddress%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("Address:            %lS\n", bstr.raw());
                    }

                    if (details != VMINFO_MACHINEREADABLE)
                        RTPrintf("\n");

                    rc = en->HasMore (&more);
                    ASSERT_RET (SUCCEEDED (rc), rc);

                    index ++;
                }
            }

            index = 0;
            /* scope */
            {
                if (details != VMINFO_MACHINEREADABLE)
                    RTPrintf ("Currently Attached USB Devices:\n\n");

                ComPtr <IUSBDeviceCollection> coll;
                CHECK_ERROR_RET (console, COMGETTER(USBDevices) (coll.asOutParam()), rc);

                ComPtr <IUSBDeviceEnumerator> en;
                CHECK_ERROR_RET (coll, Enumerate (en.asOutParam()), rc);

                BOOL more = FALSE;
                rc = en->HasMore (&more);
                ASSERT_RET (SUCCEEDED (rc), rc);

                if (!more)
                {
                    if (details != VMINFO_MACHINEREADABLE)
                        RTPrintf("<none>\n\n");
                }
                else
                while (more)
                {
                    ComPtr <IUSBDevice> dev;
                    rc = en->GetNext (dev.asOutParam());
                    ASSERT_RET (SUCCEEDED (rc), rc);

                    /* Query info. */
                    Guid id;
                    CHECK_ERROR_RET (dev, COMGETTER(Id)(id.asOutParam()), rc);
                    USHORT usVendorId;
                    CHECK_ERROR_RET (dev, COMGETTER(VendorId)(&usVendorId), rc);
                    USHORT usProductId;
                    CHECK_ERROR_RET (dev, COMGETTER(ProductId)(&usProductId), rc);
                    USHORT bcdRevision;
                    CHECK_ERROR_RET (dev, COMGETTER(Revision)(&bcdRevision), rc);

                    if (details == VMINFO_MACHINEREADABLE)
                        RTPrintf("USBAttachedUUID%d=\"%S\"\n"
                                 "USBAttachedVendorId%d=\"%#06x\"\n"
                                 "USBAttachedProductId%d=\"%#06x\"\n"
                                 "USBAttachedRevision%d=\"%#04x%02x\"\n",
                                 index + 1, id.toString().raw(),
                                 index + 1, usVendorId,
                                 index + 1, usProductId,
                                 index + 1, bcdRevision >> 8, bcdRevision & 0xff);
                    else
                        RTPrintf("UUID:               %S\n"
                                 "VendorId:           0x%04x (%04X)\n"
                                 "ProductId:          0x%04x (%04X)\n"
                                 "Revision:           %u.%u (%02u%02u)\n",
                                 id.toString().raw(),
                                 usVendorId, usVendorId, usProductId, usProductId,
                                 bcdRevision >> 8, bcdRevision & 0xff,
                                 bcdRevision >> 8, bcdRevision & 0xff);

                    /* optional stuff. */
                    Bstr bstr;
                    CHECK_ERROR_RET (dev, COMGETTER(Manufacturer)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBAttachedManufacturer%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("Manufacturer:       %lS\n", bstr.raw());
                    }
                    CHECK_ERROR_RET (dev, COMGETTER(Product)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBAttachedProduct%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("Product:            %lS\n", bstr.raw());
                    }
                    CHECK_ERROR_RET (dev, COMGETTER(SerialNumber)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBAttachedSerialNumber%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("SerialNumber:       %lS\n", bstr.raw());
                    }
                    CHECK_ERROR_RET (dev, COMGETTER(Address)(bstr.asOutParam()), rc);
                    if (!bstr.isEmpty())
                    {
                        if (details == VMINFO_MACHINEREADABLE)
                            RTPrintf("USBAttachedAddress%d=\"%lS\"\n", index + 1, bstr.raw());
                        else
                            RTPrintf("Address:            %lS\n", bstr.raw());
                    }

                    if (details != VMINFO_MACHINEREADABLE)
                        RTPrintf("\n");

                    rc = en->HasMore (&more);
                    ASSERT_RET (SUCCEEDED (rc), rc);

                    index ++;
                }
            }
        }
    } /* USB */

    /*
     * Shared folders
     */
    if (details != VMINFO_MACHINEREADABLE)
        RTPrintf("Shared folders:  ");
    uint32_t numSharedFolders = 0;
#if 0 // not yet implemented
    /* globally shared folders first */
    {
        ComPtr<ISharedFolderCollection> sfColl;
        ComPtr<ISharedFolderEnumerator> sfEnum;
        CHECK_ERROR_RET(virtualBox, COMGETTER(SharedFolders)(sfColl.asOutParam()), rc);
        CHECK_ERROR_RET(sfColl, Enumerate(sfEnum.asOutParam()), rc);
        BOOL fMore;
        sfEnum->HasMore(&fMore);
        while (fMore)
        {
            ComPtr<ISharedFolder> sf;
            CHECK_ERROR_RET(sfEnum, GetNext(sf.asOutParam()), rc);
            Bstr name, hostPath;
            sf->COMGETTER(Name)(name.asOutParam());
            sf->COMGETTER(HostPath)(hostPath.asOutParam());
            RTPrintf("Name: '%lS', Host path: '%lS' (global mapping)\n", name.raw(), hostPath.raw());
            ++numSharedFolders;
            CHECK_ERROR_RET(sfEnum, HasMore(&fMore), rc);
        }
    }
#endif
    /* now VM mappings */
    {
        ComPtr<ISharedFolderCollection> sfColl;
        ComPtr<ISharedFolderEnumerator> sfEnum;
        CHECK_ERROR_RET(machine, COMGETTER(SharedFolders)(sfColl.asOutParam()), rc);
        CHECK_ERROR_RET(sfColl, Enumerate(sfEnum.asOutParam()), rc);
        ULONG index = 0;
        BOOL fMore;
        sfEnum->HasMore(&fMore);
        while (fMore)
        {
            ComPtr<ISharedFolder> sf;
            CHECK_ERROR_RET(sfEnum, GetNext(sf.asOutParam()), rc);
            Bstr name, hostPath;
            BOOL writable;
            sf->COMGETTER(Name)(name.asOutParam());
            sf->COMGETTER(HostPath)(hostPath.asOutParam());
            sf->COMGETTER(Writable)(&writable);
            if (!numSharedFolders && details != VMINFO_MACHINEREADABLE)
                RTPrintf("\n\n");
            if (details == VMINFO_MACHINEREADABLE)
            {
                RTPrintf("SharedFolderNameMachineMapping%d=\"%lS\"\n", index + 1,
                         name.raw());
                RTPrintf("SharedFolderPathMachineMapping%d=\"%lS\"\n", index + 1,
                         hostPath.raw());
            }
            else
                RTPrintf("Name: '%lS', Host path: '%lS' (machine mapping), %s\n",
                         name.raw(), hostPath.raw(), writable ? "writable" : "readonly");
            ++numSharedFolders;
            CHECK_ERROR_RET(sfEnum, HasMore(&fMore), rc);
        }
    }
    /* transient mappings */
    if (console)
    {
        ComPtr<ISharedFolderCollection> sfColl;
        ComPtr<ISharedFolderEnumerator> sfEnum;
        CHECK_ERROR_RET(console, COMGETTER(SharedFolders)(sfColl.asOutParam()), rc);
        CHECK_ERROR_RET(sfColl, Enumerate(sfEnum.asOutParam()), rc);
        ULONG index = 0;
        BOOL fMore;
        sfEnum->HasMore(&fMore);
        while (fMore)
        {
            ComPtr<ISharedFolder> sf;
            CHECK_ERROR_RET(sfEnum, GetNext(sf.asOutParam()), rc);
            Bstr name, hostPath;
            sf->COMGETTER(Name)(name.asOutParam());
            sf->COMGETTER(HostPath)(hostPath.asOutParam());
            if (!numSharedFolders && details != VMINFO_MACHINEREADABLE)
                RTPrintf("\n\n");
            if (details == VMINFO_MACHINEREADABLE)
            {
                RTPrintf("SharedFolderNameTransientMapping%d=\"%lS\"\n", index + 1,
                         name.raw());
                RTPrintf("SharedFolderPathTransientMapping%d=\"%lS\"\n", index + 1,
                         hostPath.raw());
            }
            else
                RTPrintf("Name: '%lS', Host path: '%lS' (transient mapping)\n", name.raw(), hostPath.raw());
            ++numSharedFolders;
            CHECK_ERROR_RET(sfEnum, HasMore(&fMore), rc);
        }
    }
    if (!numSharedFolders && details != VMINFO_MACHINEREADABLE)
        RTPrintf("<none>\n");
    if (details != VMINFO_MACHINEREADABLE)
        RTPrintf("\n");

    if (console)
    {
        /*
         * Live VRDP info.
         */
        ComPtr<IRemoteDisplayInfo> remoteDisplayInfo;
        CHECK_ERROR_RET(console, COMGETTER(RemoteDisplayInfo)(remoteDisplayInfo.asOutParam()), rc);
        BOOL    Active;
        ULONG   NumberOfClients;
        LONG64  BeginTime;
        LONG64  EndTime;
        ULONG64 BytesSent;
        ULONG64 BytesSentTotal;
        ULONG64 BytesReceived;
        ULONG64 BytesReceivedTotal;
        Bstr    User;
        Bstr    Domain;
        Bstr    ClientName;
        Bstr    ClientIP;
        ULONG   ClientVersion;
        ULONG   EncryptionStyle;

        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(Active)             (&Active), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(NumberOfClients)    (&NumberOfClients), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(BeginTime)          (&BeginTime), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(EndTime)            (&EndTime), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(BytesSent)          (&BytesSent), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(BytesSentTotal)     (&BytesSentTotal), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(BytesReceived)      (&BytesReceived), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(BytesReceivedTotal) (&BytesReceivedTotal), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(User)               (User.asOutParam ()), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(Domain)             (Domain.asOutParam ()), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(ClientName)         (ClientName.asOutParam ()), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(ClientIP)           (ClientIP.asOutParam ()), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(ClientVersion)      (&ClientVersion), rc);
        CHECK_ERROR_RET(remoteDisplayInfo, COMGETTER(EncryptionStyle)    (&EncryptionStyle), rc);

        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("VRDPActiveConnection=\"%s\"\n", Active ? "on": "off");
        else
            RTPrintf("VRDP Connection:    %s\n", Active? "active": "not active");

        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("VRDPClients=%d\n", NumberOfClients);
        else
            RTPrintf("Clients so far:     %d\n", NumberOfClients);

        if (NumberOfClients > 0)
        {
            char timestr[128];

            if (Active)
            {
                makeTimeStr (timestr, sizeof (timestr), BeginTime);
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("VRDPStartTime=\"%s\"\n", timestr);
                else
                    RTPrintf("Start time:         %s\n", timestr);
            }
            else
            {
                makeTimeStr (timestr, sizeof (timestr), BeginTime);
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("VRDPLastStartTime=\"%s\"\n", timestr);
                else
                    RTPrintf("Last started:       %s\n", timestr);
                makeTimeStr (timestr, sizeof (timestr), EndTime);
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("VRDPLastEndTime=\"%s\"\n", timestr);
                else
                    RTPrintf("Last ended:         %s\n", timestr);
            }

            uint64_t ThroughputSend = 0;
            uint64_t ThroughputReceive = 0;
            if (EndTime != BeginTime)
            {
                ThroughputSend = (BytesSent * 1000) / (EndTime - BeginTime);
                ThroughputReceive = (BytesReceived * 1000) / (EndTime - BeginTime);
            }

            if (details == VMINFO_MACHINEREADABLE)
            {
                RTPrintf("VRDPBytesSent=%llu\n", BytesSent);
                RTPrintf("VRDPThroughputSend=%llu\n", ThroughputSend);
                RTPrintf("VRDPBytesSentTotal=%llu\n", BytesSentTotal);

                RTPrintf("VRDPBytesReceived=%llu\n", BytesReceived);
                RTPrintf("VRDPThroughputReceive=%llu\n", ThroughputReceive);
                RTPrintf("VRDPBytesReceivedTotal=%llu\n", BytesReceivedTotal);
            }
            else
            {
                RTPrintf("Sent:               %llu Bytes\n", BytesSent);
                RTPrintf("Average speed:      %llu B/s\n", ThroughputSend);
                RTPrintf("Sent total:         %llu Bytes\n", BytesSentTotal);

                RTPrintf("Received:           %llu Bytes\n", BytesReceived);
                RTPrintf("Speed:              %llu B/s\n", ThroughputReceive);
                RTPrintf("Received total:     %llu Bytes\n", BytesReceivedTotal);
            }

            if (Active)
            {
                if (details == VMINFO_MACHINEREADABLE)
                {
                    RTPrintf("VRDPUserName=\"%lS\"\n", User.raw());
                    RTPrintf("VRDPDomain=\"%lS\"\n", Domain.raw());
                    RTPrintf("VRDPClientName=\"%lS\"\n", ClientName.raw());
                    RTPrintf("VRDPClientIP=\"%lS\"\n", ClientIP.raw());
                    RTPrintf("VRDPClientVersion=%d\n",  ClientVersion);
                    RTPrintf("VRDPEncryption=\"%s\"\n", EncryptionStyle == 0? "RDP4": "RDP5 (X.509)");
                }
                else
                {
                    RTPrintf("User name:          %lS\n", User.raw());
                    RTPrintf("Domain:             %lS\n", Domain.raw());
                    RTPrintf("Client name:        %lS\n", ClientName.raw());
                    RTPrintf("Client IP:          %lS\n", ClientIP.raw());
                    RTPrintf("Client version:     %d\n",  ClientVersion);
                    RTPrintf("Encryption:         %s\n", EncryptionStyle == 0? "RDP4": "RDP5 (X.509)");
                }
            }
        }

        if (details != VMINFO_MACHINEREADABLE)
            RTPrintf("\n");
    }

    if (    details == VMINFO_STANDARD
        ||  details == VMINFO_FULL
        ||  details == VMINFO_MACHINEREADABLE)
    {
        Bstr description;
        machine->COMGETTER(Description)(description.asOutParam());
        if (!description.isEmpty())
        {
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("description=\"%lS\"\n", description.raw());
            else
                RTPrintf("Description:\n%lS\n", description.raw());
        }
    }

    ULONG guestVal;
    if (details != VMINFO_MACHINEREADABLE)
        RTPrintf("Guest:\n\n");

#ifdef VBOX_WITH_MEM_BALLOONING
    rc = machine->COMGETTER(MemoryBalloonSize)(&guestVal);
    if (SUCCEEDED(rc))
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("GuestMemoryBalloon=%d\n", guestVal);
        else
            RTPrintf("Configured memory balloon size:      %d MB\n", guestVal);
    }
#endif
    rc = machine->COMGETTER(StatisticsUpdateInterval)(&guestVal);
    if (SUCCEEDED(rc))
    {
        if (details == VMINFO_MACHINEREADABLE)
            RTPrintf("GuestStatisticsUpdateInterval=%d\n", guestVal);
        else
        {
            if (guestVal == 0)
                RTPrintf("Statistics update:                   disabled\n");
            else
                RTPrintf("Statistics update interval:          %d seconds\n", guestVal);
        }
    }
    if (details != VMINFO_MACHINEREADABLE)
        RTPrintf("\n");

    if (    console
        &&  (   details == VMINFO_STATISTICS
             || details == VMINFO_FULL
             || details == VMINFO_MACHINEREADABLE))
    {
        ComPtr <IGuest> guest;

        rc = console->COMGETTER(Guest)(guest.asOutParam());
        if (SUCCEEDED(rc))
        {
            ULONG statVal;

            rc = guest->GetStatistic(0, GuestStatisticType_SampleNumber, &statVal);
            if (details == VMINFO_MACHINEREADABLE)
                RTPrintf("StatGuestSample=%d\n", statVal);
            else
                RTPrintf("Guest statistics for sample %d:\n\n", statVal);

            rc = guest->GetStatistic(0, GuestStatisticType_CPULoad_Idle, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestLoadIdleCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: CPU Load Idle          %-3d%%\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_CPULoad_Kernel, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestLoadKernelCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: CPU Load Kernel        %-3d%%\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_CPULoad_User, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestLoadUserCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: CPU Load User          %-3d%%\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_Threads, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestThreadsCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Threads                %d\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_Processes, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestProcessesCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Processes              %d\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_Handles, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestHandlesCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Handles                %d\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_MemoryLoad, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryLoadCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Memory Load            %d%%\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_PhysMemTotal, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryTotalPhysCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Total physical memory  %-4d MB\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_PhysMemAvailable, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryFreePhysCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Free physical memory   %-4d MB\n", 0, statVal);
            }

#ifdef VBOX_WITH_MEM_BALLOONING
            rc = guest->GetStatistic(0, GuestStatisticType_PhysMemBalloon, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryBalloonCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Memory balloon size    %-4d MB\n", 0, statVal);
            }
#endif
            rc = guest->GetStatistic(0, GuestStatisticType_MemCommitTotal, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryCommittedCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Committed memory       %-4d MB\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_MemKernelTotal, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryTotalKernelCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Total kernel memory    %-4d MB\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_MemKernelPaged, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryPagedKernelCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Paged kernel memory    %-4d MB\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_MemKernelNonpaged, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestMemoryNonpagedKernelCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Nonpaged kernel memory %-4d MB\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_MemSystemCache, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestSystemCacheSizeCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: System cache size      %-4d MB\n", 0, statVal);
            }

            rc = guest->GetStatistic(0, GuestStatisticType_PageFileSize, &statVal);
            if (SUCCEEDED(rc))
            {
                if (details == VMINFO_MACHINEREADABLE)
                    RTPrintf("StatGuestPageFileSizeCPU%d=%d\n", 0, statVal);
                else
                    RTPrintf("CPU%d: Page file size         %-4d MB\n", 0, statVal);
            }

            RTPrintf("\n");
        }
        else
        {
            if (details != VMINFO_MACHINEREADABLE)
            {
                RTPrintf("[!] FAILED calling console->getGuest at line %d!\n", __LINE__);
                PRINT_RC_MESSAGE(rc);
            }
        }
    }

    /*
     * snapshots
     */
    ComPtr<ISnapshot> snapshot;
    rc = machine->GetSnapshot(Guid(), snapshot.asOutParam());
    if (SUCCEEDED(rc) && snapshot)
    {
        if (details != VMINFO_MACHINEREADABLE)
            RTPrintf("Snapshots:\n\n");
        showSnapshots(snapshot, details);
    }

    if (details != VMINFO_MACHINEREADABLE)
        RTPrintf("\n");
    return S_OK;
}

int handleShowVMInfo(int argc, char *argv[],
                     ComPtr<IVirtualBox> virtualBox, ComPtr<ISession> session)
{
    HRESULT rc;

    /* at least one option: the UUID or name of the VM */
    if (argc < 1)
        return errorSyntax(USAGE_SHOWVMINFO, "Incorrect number of parameters");

    /* try to find the given machine */
    ComPtr <IMachine> machine;
    Guid uuid (argv[0]);
    if (!uuid.isEmpty())
    {
        CHECK_ERROR (virtualBox, GetMachine (uuid, machine.asOutParam()));
    }
    else
    {
        CHECK_ERROR (virtualBox, FindMachine (Bstr(argv[0]), machine.asOutParam()));
        if (SUCCEEDED (rc))
            machine->COMGETTER(Id) (uuid.asOutParam());
    }
    if (FAILED (rc))
        return 1;

    /* 2nd option can be -details, -statistics or -argdump */
    VMINFO_DETAILS details = VMINFO_NONE;
    bool fDetails = false;
    bool fStatistics = false;
    bool fMachinereadable = false;
    for (int i=1;i<argc;i++)
    {
        if (!strcmp(argv[i], "-details"))
            fDetails = true;
        else
        if (!strcmp(argv[i], "-statistics"))
            fStatistics = true;
        if (!strcmp(argv[1], "-machinereadable"))
            fMachinereadable = true;
    }
    if (fMachinereadable)
        details = VMINFO_MACHINEREADABLE;
    else
    if (fDetails && fStatistics)
        details = VMINFO_FULL;
    else
    if (fDetails)
        details = VMINFO_STANDARD;
    else
    if (fStatistics)
        details = VMINFO_STATISTICS;

    ComPtr <IConsole> console;

    /* open an existing session for the VM */
    rc = virtualBox->OpenExistingSession (session, uuid);
    if (SUCCEEDED(rc))
        /* get the session machine */
        rc = session->COMGETTER(Machine)(machine.asOutParam());
    if (SUCCEEDED(rc))
        /* get the session console */
        rc = session->COMGETTER(Console)(console.asOutParam());

    rc = showVMInfo (virtualBox, machine, console, details);

    if (console)
        session->Close();

    return SUCCEEDED (rc) ? 0 : 1;
}

int handleList(int argc, char *argv[],
               ComPtr<IVirtualBox> virtualBox, ComPtr<ISession> session)
{
    HRESULT rc = S_OK;

    /* exactly one option: the object */
    if (argc != 1)
        return errorSyntax(USAGE_LIST, "Incorrect number of parameters");

    /* which object? */
    if (strcmp(argv[0], "vms") == 0)
    {
        /*
         * Get the list of all registered VMs
         */
        com::SafeIfaceArray <IMachine> machines;
        rc = virtualBox->COMGETTER(Machines2)(ComSafeArrayAsOutParam (machines));
        if (SUCCEEDED(rc))
        {
            /*
             * Iterate through the collection
             */
            for (size_t i = 0; i < machines.size(); ++ i)
            {
                if (machines [i])
                    rc = showVMInfo(virtualBox, machines [i]);
            }
        }
    }
    else
    if (strcmp(argv[0], "runningvms") == 0)
    {
        /*
         * Get the list of all _running_ VMs
         */
        com::SafeIfaceArray <IMachine> machines;
        rc = virtualBox->COMGETTER(Machines2)(ComSafeArrayAsOutParam (machines));
        if (SUCCEEDED(rc))
        {
            /*
             * Iterate through the collection
             */
            for (size_t i = 0; i < machines.size(); ++ i)
            {
                if (machines [i])
                {
                    MachineState_T machineState;
                    rc = machines [i]->COMGETTER(State)(&machineState);
                    if (SUCCEEDED(rc))
                    {
                        switch (machineState)
                        {
                            case MachineState_Running:
                            case MachineState_Paused:
                                {
                                    Guid uuid;
                                    rc = machines [i]->COMGETTER(Id) (uuid.asOutParam());
                                    if (SUCCEEDED(rc))
                                        RTPrintf ("%s\n", uuid.toString().raw());
                                    break;
                                }
                        }
                    }
                }
            }
        }
    }
    else
    if (strcmp(argv[0], "ostypes") == 0)
    {
        ComPtr<IGuestOSTypeCollection> coll;
        ComPtr<IGuestOSTypeEnumerator> enumerator;
        CHECK_ERROR(virtualBox, COMGETTER(GuestOSTypes)(coll.asOutParam()));
        if (SUCCEEDED(rc) && coll)
        {
            CHECK_ERROR(coll, Enumerate(enumerator.asOutParam()));
            BOOL hasMore;
            while (SUCCEEDED(enumerator->HasMore(&hasMore)) && hasMore)
            {
                ComPtr<IGuestOSType> guestOS;
                CHECK_RC_BREAK(enumerator->GetNext(guestOS.asOutParam()));
                Bstr guestId;
                guestOS->COMGETTER(Id)(guestId.asOutParam());
                RTPrintf("ID:          %lS\n", guestId.raw());
                Bstr guestDescription;
                guestOS->COMGETTER(Description)(guestDescription.asOutParam());
                RTPrintf("Description: %lS\n\n", guestDescription.raw());
            }
        }
    }
    else
    if (strcmp(argv[0], "hostdvds") == 0)
    {
        ComPtr<IHost> host;
        CHECK_ERROR(virtualBox, COMGETTER(Host)(host.asOutParam()));
        ComPtr<IHostDVDDriveCollection> coll;
        ComPtr<IHostDVDDriveEnumerator> enumerator;
        CHECK_ERROR(host, COMGETTER(DVDDrives)(coll.asOutParam()));
        if (SUCCEEDED(rc) && coll)
        {
            CHECK_ERROR(coll, Enumerate(enumerator.asOutParam()));
            BOOL hasMore;
            while (SUCCEEDED(enumerator->HasMore(&hasMore)) && hasMore)
            {
                ComPtr<IHostDVDDrive> dvdDrive;
                CHECK_RC_BREAK(enumerator->GetNext(dvdDrive.asOutParam()));
                Bstr name;
                dvdDrive->COMGETTER(Name)(name.asOutParam());
                RTPrintf("Name:        %lS\n\n", name.raw());
            }
        }
    }
    else
    if (strcmp(argv[0], "hostfloppies") == 0)
    {
        ComPtr<IHost> host;
        CHECK_ERROR(virtualBox, COMGETTER(Host)(host.asOutParam()));
        ComPtr<IHostFloppyDriveCollection> coll;
        ComPtr<IHostFloppyDriveEnumerator> enumerator;
        CHECK_ERROR(host, COMGETTER(FloppyDrives)(coll.asOutParam()));
        if (SUCCEEDED(rc) && coll)
        {
            CHECK_ERROR(coll, Enumerate(enumerator.asOutParam()));
            BOOL hasMore;
            while (SUCCEEDED(enumerator->HasMore(&hasMore)) && hasMore)
            {
                ComPtr<IHostFloppyDrive> floppyDrive;
                CHECK_RC_BREAK(enumerator->GetNext(floppyDrive.asOutParam()));
                Bstr name;
                floppyDrive->COMGETTER(Name)(name.asOutParam());
                RTPrintf("Name:        %lS\n\n", name.raw());
            }
        }
    }
    else
    if (strcmp(argv[0], "hostifs") == 0)
    {
        ComPtr<IHost> host;
        CHECK_ERROR(virtualBox, COMGETTER(Host)(host.asOutParam()));
        ComPtr<IHostNetworkInterfaceCollection> coll;
        ComPtr<IHostNetworkInterfaceEnumerator> enumerator;
        CHECK_ERROR(host, COMGETTER(NetworkInterfaces)(coll.asOutParam()));
        if (SUCCEEDED(rc) && coll)
        {
            CHECK_ERROR(coll, Enumerate(enumerator.asOutParam()));
            BOOL hasMore;
            while (SUCCEEDED(enumerator->HasMore(&hasMore)) && hasMore)
            {
                ComPtr<IHostNetworkInterface> networkInterface;
                CHECK_RC_BREAK(enumerator->GetNext(networkInterface.asOutParam()));
                Bstr interfaceName;
                networkInterface->COMGETTER(Name)(interfaceName.asOutParam());
                RTPrintf("Name:        %lS\n", interfaceName.raw());
                Guid interfaceGuid;
                networkInterface->COMGETTER(Id)(interfaceGuid.asOutParam());
                RTPrintf("GUID:        %lS\n\n", Bstr(interfaceGuid.toString()).raw());
            }
        }
    }
    else
    if (strcmp(argv[0], "hostinfo") == 0)
    {
        ComPtr<IHost> Host;
        CHECK_ERROR (virtualBox, COMGETTER(Host)(Host.asOutParam()));

        RTPrintf("Host Information:\n\n");

        LONG64 uTCTime = 0;
        CHECK_ERROR (Host, COMGETTER(UTCTime)(&uTCTime));
        RTTIMESPEC timeSpec;
        RTTimeSpecSetMilli(&timeSpec, uTCTime);
        char pszTime[30] = {0};
        RTTimeSpecToString(&timeSpec, pszTime, sizeof(pszTime));
        RTPrintf("Host time: %s\n", pszTime);

        ULONG processorOnlineCount = 0;
        CHECK_ERROR (Host, COMGETTER(ProcessorOnlineCount)(&processorOnlineCount));
        RTPrintf("Processor online count: %lu\n", processorOnlineCount);
        ULONG processorCount = 0;
        CHECK_ERROR (Host, COMGETTER(ProcessorCount)(&processorCount));
        RTPrintf("Processor count: %lu\n", processorCount);
        ULONG processorSpeed = 0;
        Bstr processorDescription;
        for (ULONG i = 0; i < processorCount; i++)
        {
            CHECK_ERROR (Host, GetProcessorSpeed(i, &processorSpeed));
            if (processorSpeed)
                RTPrintf("Processor#%u speed: %lu MHz\n", i, processorSpeed);
            else
                RTPrintf("Processor#%u speed: unknown\n", i, processorSpeed);
    #if 0 /* not yet implemented in Main */
            CHECK_ERROR (Host, GetProcessorDescription(i, processorDescription.asOutParam()));
            RTPrintf("Processor#%u description: %lS\n", i, processorDescription.raw());
    #endif
        }

    #if 0 /* not yet implemented in Main */
        ULONG memorySize = 0;
        CHECK_ERROR (Host, COMGETTER(MemorySize)(&memorySize));
        RTPrintf("Memory size: %lu MByte\n", memorySize);

        ULONG memoryAvailable = 0;
        CHECK_ERROR (Host, COMGETTER(MemoryAvailable)(&memoryAvailable));
        RTPrintf("Memory available: %lu MByte\n", memoryAvailable);

        Bstr operatingSystem;
        CHECK_ERROR (Host, COMGETTER(OperatingSystem)(operatingSystem.asOutParam()));
        RTPrintf("Operating system: %lS\n", operatingSystem.raw());

        Bstr oSVersion;
        CHECK_ERROR (Host, COMGETTER(OSVersion)(oSVersion.asOutParam()));
        RTPrintf("Operating system version: %lS\n", oSVersion.raw());
    #endif
    }
    else
    if (strcmp(argv[0], "hddbackends") == 0)
    {
        ComPtr<ISystemProperties> systemProperties;
        CHECK_ERROR(virtualBox,
                    COMGETTER(SystemProperties) (systemProperties.asOutParam()));
        com::SafeIfaceArray <IHardDiskFormat> hardDiskFormats;
        CHECK_ERROR(systemProperties,
                    COMGETTER(HardDiskFormats) (ComSafeArrayAsOutParam (hardDiskFormats)));

        RTPrintf("Supported hard disk backends:\n\n");
        for (size_t i = 0; i < hardDiskFormats.size(); ++ i)
        {
            /* General information */
            Bstr id;
            CHECK_ERROR(hardDiskFormats [i],
                        COMGETTER(Id) (id.asOutParam()));

            Bstr description;
            CHECK_ERROR(hardDiskFormats [i],
                        COMGETTER(Id) (description.asOutParam()));

            ULONG caps;
            CHECK_ERROR(hardDiskFormats [i],
                        COMGETTER(Capabilities) (&caps));

            RTPrintf("Backend %u: id='%ls' description='%ls' capabilities=%#06x extensions='",
                     i, id.raw(), description.raw(), caps);

            /* File extensions */
            com::SafeArray <BSTR> fileExtensions;
            CHECK_ERROR(hardDiskFormats [i],
                        COMGETTER(FileExtensions) (ComSafeArrayAsOutParam (fileExtensions)));
            for (size_t a = 0; a < fileExtensions.size(); ++ a)
            {
                RTPrintf ("%ls", Bstr (fileExtensions [a]).raw());
                if (a != fileExtensions.size()-1)
                    RTPrintf (",");
            }
            RTPrintf ("'");

            /* Configuration keys */
            com::SafeArray <BSTR> propertyNames;
            com::SafeArray <BSTR> propertyDescriptions;
            com::SafeArray <DataType_T> propertyTypes;
            com::SafeArray <ULONG> propertyFlags;
            com::SafeArray <BSTR> propertyDefaults;
            CHECK_ERROR(hardDiskFormats [i],
                        DescribeProperties (ComSafeArrayAsOutParam (propertyNames),
                                            ComSafeArrayAsOutParam (propertyDescriptions),
                                            ComSafeArrayAsOutParam (propertyTypes),
                                            ComSafeArrayAsOutParam (propertyFlags),
                                            ComSafeArrayAsOutParam (propertyDefaults)));

            RTPrintf (" properties=(");
            if (propertyNames.size() > 0)
            {
                for (size_t a = 0; a < propertyNames.size(); ++ a)
                {
                    RTPrintf ("\n  name='%ls' desc='%ls' type=",
                              Bstr (propertyNames [a]).raw(), Bstr (propertyDescriptions [a]).raw());
                    switch (propertyTypes [a])
                    {
                        case DataType_Int32: RTPrintf ("int"); break;
                        case DataType_Int8: RTPrintf ("byte"); break;
                        case DataType_String: RTPrintf ("string"); break;
                    }
                    RTPrintf (" flags=%#04x", propertyFlags [a]);
                    RTPrintf (" default='%ls'", Bstr (propertyDefaults [a]).raw());
                    if (a != propertyNames.size()-1)
                        RTPrintf (", ");
                }
            }
            RTPrintf (")\n");
        }
    }
    else
    if (strcmp(argv[0], "hdds") == 0)
    {
        com::SafeIfaceArray <IHardDisk2> hdds;
        CHECK_ERROR(virtualBox, COMGETTER(HardDisks2)(ComSafeArrayAsOutParam (hdds)));
        for (size_t i = 0; i < hdds.size(); ++ i)
        {
            ComPtr<IHardDisk2> hdd = hdds[i];
            Guid uuid;
            hdd->COMGETTER(Id)(uuid.asOutParam());
            RTPrintf("UUID:         %s\n", uuid.toString().raw());
            Bstr format;
            hdd->COMGETTER(Format)(format.asOutParam());
            RTPrintf("Format:       %lS\n", format.raw());
            Bstr filepath;
            hdd->COMGETTER(Location)(filepath.asOutParam());
            RTPrintf("Location:     %lS\n", filepath.raw());
            MediaState_T enmState;
            /// @todo NEWMEDIA check accessibility of all parents
            /// @todo NEWMEDIA print the full state value
            hdd->COMGETTER(State)(&enmState);
            RTPrintf("Accessible:   %s\n", enmState != MediaState_Inaccessible ? "yes" : "no");
            com::SafeGUIDArray machineIds;
            hdd->COMGETTER(MachineIds)(ComSafeArrayAsOutParam(machineIds));
            for (size_t j = 0; j < machineIds.size(); ++ j)
            {
                ComPtr<IMachine> machine;
                CHECK_ERROR(virtualBox, GetMachine(machineIds[j], machine.asOutParam()));
                ASSERT(machine);
                Bstr name;
                machine->COMGETTER(Name)(name.asOutParam());
                machine->COMGETTER(Id)(uuid.asOutParam());
                RTPrintf("%s%lS (UUID: %RTuuid)\n",
                         j == 0 ? "Usage:        " : "              ",
                         name.raw(), &machineIds[j]);
            }
            /// @todo NEWMEDIA check usage in snapshots too
            /// @todo NEWMEDIA also list children and say 'differencing' for
            /// hard disks with the parent or 'base' otherwise.
            RTPrintf("\n");
        }
    }
    else
    if (strcmp(argv[0], "dvds") == 0)
    {
        com::SafeIfaceArray<IDVDImage2> dvds;
        CHECK_ERROR(virtualBox, COMGETTER(DVDImages)(ComSafeArrayAsOutParam(dvds)));
        for (size_t i = 0; i < dvds.size(); ++ i)
        {
            ComPtr<IDVDImage2> dvdImage = dvds[i];
            Guid uuid;
            dvdImage->COMGETTER(Id)(uuid.asOutParam());
            RTPrintf("UUID:       %s\n", uuid.toString().raw());
            Bstr filePath;
            dvdImage->COMGETTER(Location)(filePath.asOutParam());
            RTPrintf("Path:       %lS\n", filePath.raw());
            MediaState_T enmState;
            dvdImage->COMGETTER(State)(&enmState);
            RTPrintf("Accessible: %s\n", enmState != MediaState_Inaccessible ? "yes" : "no");
            /** @todo usage */
            RTPrintf("\n");
        }
    }
    else
    if (strcmp(argv[0], "floppies") == 0)
    {
        com::SafeIfaceArray<IFloppyImage2> floppies;
        CHECK_ERROR(virtualBox, COMGETTER(FloppyImages)(ComSafeArrayAsOutParam(floppies)));
        for (size_t i = 0; i < floppies.size(); ++ i)
        {
            ComPtr<IFloppyImage2> floppyImage = floppies[i];
            Guid uuid;
            floppyImage->COMGETTER(Id)(uuid.asOutParam());
            RTPrintf("UUID:       %s\n", uuid.toString().raw());
            Bstr filePath;
            floppyImage->COMGETTER(Location)(filePath.asOutParam());
            RTPrintf("Path:       %lS\n", filePath.raw());
            MediaState_T enmState;
            floppyImage->COMGETTER(State)(&enmState);
            RTPrintf("Accessible: %s\n", enmState != MediaState_Inaccessible ? "yes" : "no");
            /** @todo usage */
            RTPrintf("\n");
        }
    }
    else
    if (strcmp(argv[0], "usbhost") == 0)
    {
        ComPtr<IHost> Host;
        CHECK_ERROR_RET (virtualBox, COMGETTER(Host)(Host.asOutParam()), 1);

        ComPtr<IHostUSBDeviceCollection> CollPtr;
        CHECK_ERROR_RET (Host, COMGETTER(USBDevices)(CollPtr.asOutParam()), 1);

        ComPtr<IHostUSBDeviceEnumerator> EnumPtr;
        CHECK_ERROR_RET (CollPtr, Enumerate(EnumPtr.asOutParam()), 1);

        RTPrintf("Host USB Devices:\n\n");

        BOOL fMore = FALSE;
        rc = EnumPtr->HasMore (&fMore);
        ASSERT_RET (SUCCEEDED (rc), 1);

        if (!fMore)
        {
            RTPrintf("<none>\n\n");
        }
        else
        while (fMore)
        {
            ComPtr <IHostUSBDevice> dev;
            rc = EnumPtr->GetNext (dev.asOutParam());
            ASSERT_RET (SUCCEEDED (rc), 1);

            /* Query info. */
            Guid id;
            CHECK_ERROR_RET (dev, COMGETTER(Id)(id.asOutParam()), 1);
            USHORT usVendorId;
            CHECK_ERROR_RET (dev, COMGETTER(VendorId)(&usVendorId), 1);
            USHORT usProductId;
            CHECK_ERROR_RET (dev, COMGETTER(ProductId)(&usProductId), 1);
            USHORT bcdRevision;
            CHECK_ERROR_RET (dev, COMGETTER(Revision)(&bcdRevision), 1);

            RTPrintf("UUID:               %S\n"
                     "VendorId:           0x%04x (%04X)\n"
                     "ProductId:          0x%04x (%04X)\n"
                     "Revision:           %u.%u (%02u%02u)\n",
                     id.toString().raw(),
                     usVendorId, usVendorId, usProductId, usProductId,
                     bcdRevision >> 8, bcdRevision & 0xff,
                     bcdRevision >> 8, bcdRevision & 0xff);

            /* optional stuff. */
            Bstr bstr;
            CHECK_ERROR_RET (dev, COMGETTER(Manufacturer)(bstr.asOutParam()), 1);
            if (!bstr.isEmpty())
                RTPrintf("Manufacturer:       %lS\n", bstr.raw());
            CHECK_ERROR_RET (dev, COMGETTER(Product)(bstr.asOutParam()), 1);
            if (!bstr.isEmpty())
                RTPrintf("Product:            %lS\n", bstr.raw());
            CHECK_ERROR_RET (dev, COMGETTER(SerialNumber)(bstr.asOutParam()), 1);
            if (!bstr.isEmpty())
                RTPrintf("SerialNumber:       %lS\n", bstr.raw());
            CHECK_ERROR_RET (dev, COMGETTER(Address)(bstr.asOutParam()), 1);
            if (!bstr.isEmpty())
                RTPrintf("Address:            %lS\n", bstr.raw());

            /* current state  */
            USBDeviceState_T state;
            CHECK_ERROR_RET (dev, COMGETTER(State)(&state), 1);
            const char *pszState = "?";
            switch (state)
            {
                case USBDeviceState_NotSupported:
                    pszState = "Not supported"; break;
                case USBDeviceState_Unavailable:
                    pszState = "Unavailable"; break;
                case USBDeviceState_Busy:
                    pszState = "Busy"; break;
                case USBDeviceState_Available:
                    pszState = "Available"; break;
                case USBDeviceState_Held:
                    pszState = "Held"; break;
                case USBDeviceState_Captured:
                    pszState = "Captured"; break;
                default:
                    ASSERT (false);
                    break;
            }
            RTPrintf("Current State:      %s\n\n", pszState);

            rc = EnumPtr->HasMore (&fMore);
            ASSERT_RET (SUCCEEDED (rc), rc);
        }
    }
    else
    if (strcmp(argv[0], "usbfilters") == 0)
    {
        RTPrintf("Global USB Device Filters:\n\n");

        ComPtr <IHost> host;
        CHECK_ERROR_RET (virtualBox, COMGETTER(Host) (host.asOutParam()), 1);

        ComPtr<IHostUSBDeviceFilterCollection> coll;
        CHECK_ERROR_RET (host, COMGETTER (USBDeviceFilters)(coll.asOutParam()), 1);

        ComPtr<IHostUSBDeviceFilterEnumerator> en;
        CHECK_ERROR_RET (coll, Enumerate(en.asOutParam()), 1);

        ULONG index = 0;
        BOOL more = FALSE;
        rc = en->HasMore (&more);
        ASSERT_RET (SUCCEEDED (rc), 1);

        if (!more)
        {
            RTPrintf("<none>\n\n");
        }
        else
        while (more)
        {
            ComPtr<IHostUSBDeviceFilter> flt;
            rc = en->GetNext (flt.asOutParam());
            ASSERT_RET (SUCCEEDED (rc), 1);

            /* Query info. */

            RTPrintf("Index:            %lu\n", index);

            BOOL active = FALSE;
            CHECK_ERROR_RET (flt, COMGETTER (Active) (&active), 1);
            RTPrintf("Active:           %s\n", active ? "yes" : "no");

            USBDeviceFilterAction_T action;
            CHECK_ERROR_RET (flt, COMGETTER (Action) (&action), 1);
            const char *pszAction = "<invalid>";
            switch (action)
            {
                case USBDeviceFilterAction_Ignore:
                    pszAction = "Ignore";
                    break;
                case USBDeviceFilterAction_Hold:
                    pszAction = "Hold";
                    break;
                default:
                    break;
            }
            RTPrintf("Action:           %s\n", pszAction);

            Bstr bstr;
            CHECK_ERROR_RET (flt, COMGETTER (Name) (bstr.asOutParam()), 1);
            RTPrintf("Name:             %lS\n", bstr.raw());
            CHECK_ERROR_RET (flt, COMGETTER (VendorId) (bstr.asOutParam()), 1);
            RTPrintf("VendorId:         %lS\n", bstr.raw());
            CHECK_ERROR_RET (flt, COMGETTER (ProductId) (bstr.asOutParam()), 1);
            RTPrintf("ProductId:        %lS\n", bstr.raw());
            CHECK_ERROR_RET (flt, COMGETTER (Revision) (bstr.asOutParam()), 1);
            RTPrintf("Revision:         %lS\n", bstr.raw());
            CHECK_ERROR_RET (flt, COMGETTER (Manufacturer) (bstr.asOutParam()), 1);
            RTPrintf("Manufacturer:     %lS\n", bstr.raw());
            CHECK_ERROR_RET (flt, COMGETTER (Product) (bstr.asOutParam()), 1);
            RTPrintf("Product:          %lS\n", bstr.raw());
            CHECK_ERROR_RET (flt, COMGETTER (SerialNumber) (bstr.asOutParam()), 1);
            RTPrintf("Serial Number:    %lS\n\n", bstr.raw());

            rc = en->HasMore (&more);
            ASSERT_RET (SUCCEEDED (rc), 1);

            index ++;
        }
    }
    else if (strcmp(argv[0], "systemproperties") == 0)
    {
        ComPtr<ISystemProperties> systemProperties;
        virtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());

        Bstr str;
        ULONG ulValue;
        ULONG64 ul64Value;
        BOOL flag;

        systemProperties->COMGETTER(MinGuestRAM)(&ulValue);
        RTPrintf("Minimum guest RAM size:      %u Megabytes\n", ulValue);
        systemProperties->COMGETTER(MaxGuestRAM)(&ulValue);
        RTPrintf("Maximum guest RAM size:      %u Megabytes\n", ulValue);
        systemProperties->COMGETTER(MaxGuestVRAM)(&ulValue);
        RTPrintf("Maximum video RAM size:      %u Megabytes\n", ulValue);
        systemProperties->COMGETTER(MaxVDISize)(&ul64Value);
        RTPrintf("Maximum VDI size:            %lu Megabytes\n", ul64Value);
        systemProperties->COMGETTER(DefaultHardDiskFolder)(str.asOutParam());
        RTPrintf("Default hard disk filder:    %lS\n", str.raw());
        systemProperties->COMGETTER(DefaultMachineFolder)(str.asOutParam());
        RTPrintf("Default machine folder:      %lS\n", str.raw());
        systemProperties->COMGETTER(RemoteDisplayAuthLibrary)(str.asOutParam());
        RTPrintf("VRDP authentication library: %lS\n", str.raw());
        systemProperties->COMGETTER(WebServiceAuthLibrary)(str.asOutParam());
        RTPrintf("Webservice auth. library:    %lS\n", str.raw());
        systemProperties->COMGETTER(HWVirtExEnabled)(&flag);
        RTPrintf("Hardware virt. extensions:   %s\n", flag ? "yes" : "no");
        systemProperties->COMGETTER(LogHistoryCount)(&ulValue);
        RTPrintf("Log history count:           %u\n", ulValue);

    }
    else
        return errorSyntax(USAGE_LIST, "Invalid parameter '%s'", Utf8Str(argv[0]).raw());

    return SUCCEEDED(rc) ? 0 : 1;
}

#endif /* VBOX_ONLY_DOCS */

