/** @file
  Header file for 'tftp' command functions.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TFTP_H_
#define _TFTP_H_

#include <Uefi.h>

#include <Protocol/HiiPackageList.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Mtftp4.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiHiiServicesLib.h>

extern EFI_HANDLE mTftpHiiHandle;

typedef struct {
  UINTN  FileSize;
  UINTN  DownloadedNbOfBytes;
  UINTN  LastReportedNbOfBytes;
} DOWNLOAD_CONTEXT;

/**
  Function for 'tftp' command.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval SHELL_SUCCESS            Command completed successfully.
  @retval SHELL_INVALID_PARAMETER  Command usage error.
  @retval SHELL_ABORTED            The user aborts the operation.
  @retval value                    Unknown error.
**/
SHELL_STATUS
RunTftp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Retrive HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HANDLE
InitializeHiiPackage (
  EFI_HANDLE                  ImageHandle
  );
#endif // _TFTP_H_
