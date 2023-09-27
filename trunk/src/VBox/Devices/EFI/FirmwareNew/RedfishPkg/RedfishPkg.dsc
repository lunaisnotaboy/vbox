## @file
# Redfish Package
#
# Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2021 Hewlett-Packard Enterprise Development LP.
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = RedfishPkg
  PLATFORM_GUID                  = c4352870-5232-11e7-9522-005056c00008
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/RedfishPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  RedfishPlatformHostInterfaceLib|RedfishPkg/Library/PlatformHostInterfaceLibNull/PlatformHostInterfaceLibNull.inf
  HttpLib|NetworkPkg/Library/DxeHttpLib/DxeHttpLib.inf
  HttpIoLib|NetworkPkg/Library/DxeHttpIoLib/DxeHttpIoLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  DpcLib|NetworkPkg/Library/DxeDpcLib/DxeDpcLib.inf
  RedfishPlatformCredentialLib|RedfishPkg/Library/PlatformCredentialLibNull/PlatformCredentialLibNull.inf
  RedfishContentCodingLib|RedfishPkg/Library/RedfishContentCodingLibNull/RedfishContentCodingLibNull.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  #
  # This library provides the instrinsic functions generated by a given compiler.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf
  ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf

[Components]
  RedfishPkg/Library/PlatformHostInterfaceLibNull/PlatformHostInterfaceLibNull.inf
  RedfishPkg/Library/PlatformCredentialLibNull/PlatformCredentialLibNull.inf
  RedfishPkg/Library/RedfishContentCodingLibNull/RedfishContentCodingLibNull.inf
  RedfishPkg/Library/DxeRestExLib/DxeRestExLib.inf
  RedfishPkg/Library/BaseUcs2Utf8Lib/BaseUcs2Utf8Lib.inf
  RedfishPkg/PrivateLibrary/RedfishCrtLib/RedfishCrtLib.inf
  RedfishPkg/Library/JsonLib/JsonLib.inf
  RedfishPkg/PrivateLibrary/RedfishLib/RedfishLib.inf

  !include RedfishPkg/Redfish.dsc.inc
