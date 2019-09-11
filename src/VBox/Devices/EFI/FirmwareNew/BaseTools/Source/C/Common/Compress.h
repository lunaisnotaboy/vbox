/** @file
Header file for compression routine.
Providing both EFI and Tiano Compress algorithms.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _COMPRESS_H_
#define _COMPRESS_H_

#include <string.h>
#include <stdlib.h>

#include "CommonLib.h"
#include <Common/UefiBaseTypes.h>
/*++

Routine Description:

  Tiano compression routine.

--*/
EFI_STATUS
TianoCompress (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize
  )
;

/*++

Routine Description:

  Efi compression routine.

--*/
EFI_STATUS
EfiCompress (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize
  )
;

/*++

Routine Description:

  The compression routine.

Arguments:

  SrcBuffer   - The buffer storing the source data
  SrcSize     - The size of source data
  DstBuffer   - The buffer to store the compressed data
  DstSize     - On input, the size of DstBuffer; On output,
                the size of the actual compressed data.

Returns:

  EFI_BUFFER_TOO_SMALL  - The DstBuffer is too small. In this case,
                DstSize contains the size needed.
  EFI_SUCCESS           - Compression is successful.
  EFI_OUT_OF_RESOURCES  - No resource to complete function.
  EFI_INVALID_PARAMETER - Parameter supplied is wrong.

--*/
typedef
EFI_STATUS
(*COMPRESS_FUNCTION) (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize
  );

#endif
