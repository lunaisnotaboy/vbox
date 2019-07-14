/* $Id$ */
/** @file
 * DHCP server - DHCP options
 */

/*
 * Copyright (C) 2017-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "DhcpdInternal.h"
#include "DhcpOptions.h"
#include "DhcpMessage.h"

#include <iprt/cidr.h>


optmap_t &operator<<(optmap_t &optmap, DhcpOption *option)
{
    if (option == NULL)
        return optmap;

    if (option->present())
        optmap[option->optcode()] = std::shared_ptr<DhcpOption>(option);
    else
        optmap.erase(option->optcode());

    return optmap;
}


optmap_t &operator<<(optmap_t &optmap, const std::shared_ptr<DhcpOption> &option)
{
    if (!option)
        return optmap;

    if (option->present())
        optmap[option->optcode()] = option;
    else
        optmap.erase(option->optcode());

    return optmap;
}


int DhcpOption::encode(octets_t &dst) const
{
    if (!m_fPresent)
        return VERR_INVALID_STATE;

    size_t cbOrig = dst.size();

    append(dst, m_OptCode);
    appendLength(dst, 0);  /* placeholder */

    ssize_t cbValue = encodeValue(dst);
    if (cbValue < 0 || UINT8_MAX <= cbValue)
    {
        dst.resize(cbOrig); /* undo */
        return VERR_INVALID_PARAMETER;
    }

    dst[cbOrig+1] = cbValue;
    return VINF_SUCCESS;
}


/* static */
const octets_t *DhcpOption::findOption(const rawopts_t &aOptMap, uint8_t aOptCode)
{
    rawopts_t::const_iterator it(aOptMap.find(aOptCode));
    if (it == aOptMap.end())
        return NULL;

    return &it->second;
}


int DhcpOption::decode(const rawopts_t &map)
{
    const octets_t *rawopt = DhcpOption::findOption(map, m_OptCode);
    if (rawopt == NULL)
        return VERR_NOT_FOUND;

    int rc = decodeValue(*rawopt, rawopt->size());
    if (RT_FAILURE(rc))
        return VERR_INVALID_PARAMETER;

    return VINF_SUCCESS;
}


int DhcpOption::decode(const DhcpClientMessage &req)
{
    return decode(req.rawopts());
}


int DhcpOption::parse1(bool &aValue, const char *pcszValue)
{
    pcszValue = RTStrStripL(pcszValue);
    if (   strcmp(pcszValue, "true") == 0
        || strcmp(pcszValue, "1")    == 0
        || strcmp(pcszValue, "yes")  == 0
        || strcmp(pcszValue, "on")   == 0 )
    {
        aValue = true;
        return VINF_SUCCESS;
    }

    if (   strcmp(pcszValue, "false") == 0
        || strcmp(pcszValue, "0")     == 0
        || strcmp(pcszValue, "no")    == 0
        || strcmp(pcszValue, "off")   == 0 )
    {
        aValue = false;
        return VINF_SUCCESS;
    }

    uint8_t bTmp;
    int rc = RTStrToUInt8Full(RTStrStripL(pcszValue), 10, &bTmp);
    if (rc == VERR_TRAILING_SPACES)
        rc = VINF_SUCCESS;
    if (RT_SUCCESS(rc))
        aValue = bTmp != 0;

    return rc;
}


int DhcpOption::parse1(uint8_t &aValue, const char *pcszValue)
{
    int rc = RTStrToUInt8Full(RTStrStripL(pcszValue), 10, &aValue);
    if (rc == VERR_TRAILING_SPACES)
        rc = VINF_SUCCESS;
    return rc;
}


int DhcpOption::parse1(uint16_t &aValue, const char *pcszValue)
{
    int rc = RTStrToUInt16Full(RTStrStripL(pcszValue), 10, &aValue);

    if (rc == VERR_TRAILING_SPACES)
        rc = VINF_SUCCESS;
    return rc;
}


int DhcpOption::parse1(uint32_t &aValue, const char *pcszValue)
{
    int rc = RTStrToUInt32Full(RTStrStripL(pcszValue), 10, &aValue);

    if (rc == VERR_TRAILING_SPACES)
        rc = VINF_SUCCESS;
    return rc;
}


int DhcpOption::parse1(RTNETADDRIPV4 &aValue, const char *pcszValue)
{
    return RTNetStrToIPv4Addr(pcszValue, &aValue);
}


int DhcpOption::parse1(DhcpIpv4AddrAndMask &aValue, const char *pcszValue)
{
    return RTCidrStrToIPv4(pcszValue, &aValue.Ipv4, &aValue.Mask);
}


template <typename a_Type>
static int DhcpOption::parseList(std::vector<a_Type> &aList, const char *pcszValue)
{
    std::vector<a_Type> vecTmp;

    pcszValue = RTStrStripL(pcszValue);
    for (;;)
    {
        /* Assume space, tab, comma or semicolon is used as separator (superset of RTStrStrip): */
        const char *pszNext = strpbrk(pcszValue, " ,;:\t\n\r");
        char szTmp[256];
        if (pszNext)
        {
            size_t cchToCopy = pszNext - pcszValue;
            if (cchToCopy >= sizeof(szTmp))
                return VERR_INVALID_PARAMETER;
            memcpy(szTmp, pcszValue, cchToCopy);
            szTmp[cchToCopy] = '\0';
            pcszValue = szTmp;

            /* Advance pszNext past the separator character and fluff: */
            char ch;
            do
                pszNext++;
            while ((ch = *pszNext) == ' ' || ch == ':' || ch == ';' || ch == '\t' || ch == '\n' || ch == '\r');
            if (ch == '\0')
                pszNext = NULL;
        }

        /* Try convert it: */
        a_Type Value;
        int rc = DhcpOption::parse1(Value, pcszValue);
        if (RT_SUCCESS(rc))
            vecTmp.push_back(Value);
        else
            return VERR_INVALID_PARAMETER;

        if (pszNext)
            pcszValue = pszNext;
        else
            break;
    }

    aList.swap(vecTmp);
    return VINF_SUCCESS;

}

/** ASSUME that uint8_t means hex byte strings. */
template <>
static int DhcpOption::parseList(std::vector<uint8_t> &aList, const char *pcszValue)
{
    uint8_t     abBuf[256];
    const char *pszNext = NULL;
    size_t      cbReturned = 0;
    int rc = RTStrConvertHexBytesEx(RTStrStripL(pcszValue), abBuf, sizeof(abBuf), RTSTRCONVERTHEXBYTES_F_SEP_COLON,
                                    &pszNext, &cbReturned);
    if (RT_SUCCESS(rc))
    {
        if (pszNext)
            pszNext = RTStrStripL(pszNext);
        if (*pszNext)
        {
            for (size_t i = 0; i < cbReturned; i++)
                aList.push_back(abBuf[i]);
            return VINF_SUCCESS;
        }
        rc = VERR_TRAILING_CHARS;
    }
    return rc;
}



/*
 * XXX: See DHCPServer::encodeOption()
 */
int DhcpOption::parseHex(octets_t &aRawValue, const char *pcszValue)
{
    octets_t data;
    char *pszNext;
    int rc;

    if (pcszValue == NULL || *pcszValue == '\0')
        return VERR_INVALID_PARAMETER;

    while (*pcszValue != '\0')
    {
        if (data.size() > UINT8_MAX)
            return VERR_INVALID_PARAMETER;

        uint8_t u8Byte;
        rc = RTStrToUInt8Ex(pcszValue, &pszNext, 16, &u8Byte);
        if (!RT_SUCCESS(rc))
            return rc;

        if (*pszNext == ':')
            ++pszNext;
        else if (*pszNext != '\0')
            return VERR_PARSE_ERROR;

        data.push_back(u8Byte);
        pcszValue = pszNext;
    }

    aRawValue.swap(data);
    return VINF_SUCCESS;
}


DhcpOption *DhcpOption::parse(uint8_t aOptCode, int aEnc, const char *pcszValue, int *prc /*= NULL*/)
{
    int rcIgn;
    if (!prc)
        prc = &rcIgn;

    switch (aEnc)
    {
        case 0: /* DhcpOptEncoding_Legacy */
            switch (aOptCode)
            {
#define HANDLE(a_OptClass)  \
                case a_OptClass::optcode: \
                    return a_OptClass::parse(pcszValue, prc)

                HANDLE(OptSubnetMask);                  // 1
                HANDLE(OptTimeOffset);                  // 2
                HANDLE(OptRouters);                     // 3
                HANDLE(OptTimeServers);                 // 4
                HANDLE(OptNameServers);                 // 5
                HANDLE(OptDNSes);                       // 6
                HANDLE(OptLogServers);                  // 7
                HANDLE(OptCookieServers);               // 8
                HANDLE(OptLPRServers);                  // 9
                HANDLE(OptImpressServers);              // 10
                HANDLE(OptResourceLocationServers);     // 11
                HANDLE(OptHostName);                    // 12
                HANDLE(OptBootFileSize);                // 13
                HANDLE(OptMeritDumpFile);               // 14
                HANDLE(OptDomainName);                  // 15
                HANDLE(OptSwapServer);                  // 16
                HANDLE(OptRootPath);                    // 17
                HANDLE(OptExtensionPath);               // 18
                HANDLE(OptIPForwarding);                // 19
                HANDLE(OptNonLocalSourceRouting);       // 20
                HANDLE(OptPolicyFilter);                // 21
                HANDLE(OptMaxDatagramReassemblySize);   // 22
                HANDLE(OptDefaultIPTTL);                // 23
                HANDLE(OptDefaultPathMTUAgingTimeout);  // 24
                HANDLE(OptPathMTUPlateauTable);         // 25
                HANDLE(OptInterfaceMTU);                // 26
                HANDLE(OptAllSubnetsAreLocal);          // 27
                HANDLE(OptBroadcastAddress);            // 28
                HANDLE(OptPerformMaskDiscovery);        // 29
                HANDLE(OptMaskSupplier);                // 30
                HANDLE(OptPerformRouterDiscovery);      // 31
                HANDLE(OptRouterSolicitationAddress);   // 32
                HANDLE(OptStaticRoute);                 // 33
                HANDLE(OptTrailerEncapsulation);        // 34
                HANDLE(OptARPCacheTimeout);             // 35
                HANDLE(OptEthernetEncapsulation);       // 36
                HANDLE(OptTCPDefaultTTL);               // 37
                HANDLE(OptTCPKeepaliveInterval);        // 38
                HANDLE(OptTCPKeepaliveGarbage);         // 39
                HANDLE(OptNISDomain);                   // 40
                HANDLE(OptNISServers);                  // 41
                HANDLE(OptNTPServers);                  // 42
                HANDLE(OptVendorSpecificInfo);          // 43
                HANDLE(OptNetBIOSNameServers);          // 44
                HANDLE(OptNetBIOSDatagramServers);      // 45
                HANDLE(OptNetBIOSNodeType);             // 46
                HANDLE(OptNetBIOSScope);                // 47
                HANDLE(OptXWindowsFontServers);         // 48
                HANDLE(OptXWindowsDisplayManager);      // 49
#ifndef IN_VBOXSVC /* Don't allow these in new configs */
                // OptRequestedAddress (50) is client only and not configurable.
                HANDLE(OptLeaseTime);                   // 51 - for historical reasons?  Configuable elsewhere now.
                // OptOptionOverload (52) is part of the protocol and not configurable.
                // OptMessageType (53) is part of the protocol and not configurable.
                // OptServerId (54) is the IP address of the server and configurable elsewhere.
                // OptParameterRequest (55) is client only and not configurable.
                // OptMessage (56) is server failure message and not configurable.
                // OptMaxDHCPMessageSize (57) is client only (?) and not configurable.
                HANDLE(OptRenewalTime);                 // 58 - for historical reasons?
                HANDLE(OptRebindingTime);               // 59 - for historical reasons?
                // OptVendorClassId (60) is client only and not configurable.
                // OptClientId (61) is client only and not configurable.
#endif
                HANDLE(OptNetWareIPDomainName);         // 62
                HANDLE(OptNetWareIPInformation);        // 63
                HANDLE(OptNISPlusDomain);               // 64
                HANDLE(OptNISPlusServers);              // 65
                HANDLE(OptTFTPServer);                  // 66 - perhaps we should use an alternative way to configure these.
                HANDLE(OptBootFileName);                // 67 - perhaps we should use an alternative way to configure these.
                HANDLE(OptMobileIPHomeAgents);          // 68
                HANDLE(OptSMTPServers);                 // 69
                HANDLE(OptPOP3Servers);                 // 70
                HANDLE(OptNNTPServers);                 // 71
                HANDLE(OptWWWServers);                  // 72
                HANDLE(OptFingerServers);               // 73
                HANDLE(OptIRCServers);                  // 74
                HANDLE(OptStreetTalkServers);           // 75
                HANDLE(OptSTDAServers);                 // 76
                // OptUserClassId (77) is client only and not configurable.
                HANDLE(OptSLPDirectoryAgent);           // 78
                HANDLE(OptSLPServiceScope);             // 79
                // OptRapidCommit (80) is not configurable.

#undef HANDLE
                default:
                    if (prc)
                        *prc = VERR_NOT_IMPLEMENTED;
                    return NULL;
            }
            break;

        case 1:
            return RawOption::parse(aOptCode, pcszValue, prc);

        default:
            if (prc)
                *prc = VERR_WRONG_TYPE;
            return NULL;
    }
}
