/* $Id$ */
/** @file
 * VBoxDD - Static dtrace probes
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

provider vboxdd
{
    probe hgcmcall__enter(void *pvCmd, unsigned int idFunction, unsigned int idClient, unsigned int cbCmd);
    probe hgcmcall__completed__req(void *pvCmd, int rc);
    probe hgcmcall__completed__emt(void *pvCmd, int rc);
    probe hgcmcall__completed__done(void *pvCmd, unsigned int idFunction, unsigned int idClient, int rc);

    probe ahci__req__submit(void *pvReq, int iTxDir, uint64_t offStart, uint64_t cbXfer);
    probe ahci__req__completed(void *pvReq, int rcReq, uint64_t offStart, uint64_t cbXfer);

    probe hda__stream__setup(uint32_t idxStream, int32_t rc, uint32_t uHz, uint64_t cTicksPeriod, uint32_t cbPeriod);
    probe hda__stream__reset(uint32_t idxStream);
    probe hda__stream__dma__out(uint32_t idxStream, uint32_t cb, uint64_t off);
    probe hda__stream__dma__in(uint32_t idxStream, uint32_t cb, uint64_t off);
    probe hda__stream__aio__out(uint32_t idxStream, uint32_t cb, uint64_t off);
    probe hda__stream__aio__in(uint32_t idxStream, uint32_t cb, uint64_t off);
};

#pragma D attributes Evolving/Evolving/Common provider vboxdd provider
#pragma D attributes Private/Private/Unknown  provider vboxdd module
#pragma D attributes Private/Private/Unknown  provider vboxdd function
#pragma D attributes Evolving/Evolving/Common provider vboxdd name
#pragma D attributes Evolving/Evolving/Common provider vboxdd args

