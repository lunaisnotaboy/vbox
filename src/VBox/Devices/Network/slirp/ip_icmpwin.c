/* $Id$ */
/** @file
 * NAT - Windows ICMP API based ping proxy.
 */

/*
 * Copyright (C) 2006-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "slirp.h"
#include "ip_icmp.h"

#include <winternl.h>           /* for PIO_APC_ROUTINE &c */
#include <iphlpapi.h>
#include <icmpapi.h>

/*
 * A header of ICMP ECHO.  Intended for storage, unlike struct icmp
 * which is intended to be overlayed onto a buffer.
 */
struct icmp_echo {
    uint8_t  icmp_type;
    uint8_t  icmp_code;
    uint16_t icmp_cksum;
    uint16_t icmp_echo_id;
    uint16_t icmp_echo_seq;
};

AssertCompileSize(struct icmp_echo, 8);


struct pong {
    PNATState pData;

    struct ip reqiph;
    struct icmp_echo reqicmph;

    size_t bufsize;
    uint8_t buf[1];
};


static VOID WINAPI icmpwin_callback_apc(void *ctx, PIO_STATUS_BLOCK iob, ULONG reserved);
static VOID WINAPI icmpwin_callback_old(void *ctx);

static void icmpwin_callback(struct pong *pong);

static struct mbuf *icmpwin_get_error(struct pong *pong, int type, int code);
static struct mbuf *icmpwin_get_mbuf(PNATState pData, size_t reqsize);


/*
 * On Windows XP and Windows Server 2003 IcmpSendEcho2() callback
 * is FARPROC, but starting from Vista it's PIO_APC_ROUTINE with
 * two extra arguments.  Callbacks use WINAPI (stdcall) calling
 * convention with callee responsible for popping the arguments,
 * so to avoid stack corruption we check windows version at run
 * time and provide correct callback.
 *
 * XXX: this is system-wide, but what about multiple NAT threads?
 */
static void *pfIcmpCallback;


int
icmpwin_init(PNATState pData)
{
    if (pfIcmpCallback == NULL)
    {
        OSVERSIONINFO osvi;
        int status;

        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        status = GetVersionEx(&osvi);
        if (status == 0)
            return 1;

        if (osvi.dwMajorVersion >= 6)
            pfIcmpCallback = icmpwin_callback_apc;
        else
            pfIcmpCallback = icmpwin_callback_old;
    }

    pData->icmp_socket.sh = IcmpCreateFile();
    pData->phEvents[VBOX_ICMP_EVENT_INDEX] = CreateEvent(NULL, FALSE, FALSE, NULL);
    pData->cbIcmpBuffer = sizeof(ICMP_ECHO_REPLY) * 10;
    pData->pvIcmpBuffer = RTMemAlloc(pData->cbIcmpBuffer);

    return 0;
}


void
icmpwin_finit(PNATState pData)
{
    IcmpCloseHandle(pData->icmp_socket.sh);
    RTMemFree(pData->pvIcmpBuffer);
}


/*
 * Outgoing ping from guest.
 */
void
icmpwin_ping(PNATState pData, struct mbuf *m, int hlen)
{
    struct ip *ip = mtod(m, struct ip *);
    int icmplen = ip->ip_len - hlen;
    uint8_t ttl;
    size_t bufsize;
    struct pong *pong;
    IPAddr dst;
    IP_OPTION_INFORMATION opts;
    void *reqdata;
    size_t reqsize;
    int status;

    ttl = ip->ip_ttl;
    AssertReturnVoid(ttl > 1); /* should've been dealt with in the caller */
    --ttl;

    bufsize = sizeof(ICMP_ECHO_REPLY) + icmplen;
    pong = RTMemAlloc(RT_OFFSETOF(struct pong, buf) + bufsize);
    if (RT_UNLIKELY(pong == NULL))
        return;

    pong->pData = pData;
    pong->bufsize = bufsize;
    m_copydata(m, 0, hlen, (caddr_t)&pong->reqiph);
    m_copydata(m, hlen, sizeof(struct icmp_echo), (caddr_t)&pong->reqicmph);
    AssertReturnVoid(pong->reqicmph.icmp_type == ICMP_ECHO);

    reqsize = icmplen - sizeof(struct icmp_echo); /* just the payload */
    if (m->m_next == NULL)
    {
        /* already in single contiguous buffer */
        reqdata = mtod(m, char *) + sizeof(struct ip) + sizeof(struct icmp_echo);
    }
    else
    {
        /* use reply buffer as temporary storage */
        reqdata = pong->buf;
        m_copydata(m, sizeof(struct ip) + sizeof(struct icmp_echo),
                   reqsize, reqdata);
    }

    dst = ip->ip_dst.s_addr;

    opts.Ttl = ttl;
    opts.Tos = ip->ip_tos; /* affected by DisableUserTOSSetting key */
    opts.Flags = (ip->ip_off & IP_DF) != 0 ? IP_FLAG_DF : 0;
    opts.OptionsSize = 0;
    opts.OptionsData = 0;


    status = IcmpSendEcho2(pData->icmp_socket.sh, NULL,
                           pfIcmpCallback, pong,
                           dst, reqdata, (WORD)reqsize, &opts,
                           pong->buf, (DWORD)pong->bufsize,
                           5 * 1000 /* ms */);

    if (RT_UNLIKELY(status != 0))
    {
        Log2(("NAT: IcmpSendEcho2: unexpected status %d\n", status));
    }
    else if ((status = GetLastError()) != ERROR_IO_PENDING)
    {
        int code;

        Log2(("NAT: IcmpSendEcho2: error %d\n", status));
        switch (status) {
        case ERROR_NETWORK_UNREACHABLE:
            code = ICMP_UNREACH_NET;
            break;
        case ERROR_HOST_UNREACHABLE:
            code = ICMP_UNREACH_HOST;
            break;
        default:
            code = -1;
            break;
        }

        if (code != -1)         /* send icmp error */
        {
            struct mbuf *em = icmpwin_get_error(pong, ICMP_UNREACH, code);
            if (em != NULL)
            {
                struct ip *eip = mtod(em, struct ip *);
                eip->ip_src = alias_addr;
                ip_output(pData, NULL, em);
            }
        }
    }
    else /* success */
    {
        Log2(("NAT: pong %p for ping %RTnaipv4 id 0x%04x seq %d len %d\n",
              pong, dst,
              RT_N2H_U16(pong->reqicmph.icmp_echo_id),
              RT_N2H_U16(pong->reqicmph.icmp_echo_seq),
              reqsize));
        pong = NULL;            /* callback owns it now */
    }

    if (pong != NULL)
        RTMemFree(pong);
}


static VOID WINAPI
icmpwin_callback_apc(void *ctx, PIO_STATUS_BLOCK iob, ULONG reserved)
{
    struct pong *pong = (struct pong *)ctx;

    if (pong != NULL)
    {
        icmpwin_callback(pong);
        RTMemFree(pong);
    }
}


static VOID WINAPI
icmpwin_callback_old(void *ctx)
{
    struct pong *pong = (struct pong *)ctx;

    if (pong != NULL)
    {
        icmpwin_callback(pong);
        RTMemFree(pong);
    }
}


/*
 * Actual callback code for IcmpSendEcho2().  OS version specific
 * trampoline will free "pong" argument for us.
 */
static void
icmpwin_callback(struct pong *pong)
{
    PNATState pData;
    DWORD nreplies;
    ICMP_ECHO_REPLY *reply;
    struct mbuf *m;
    struct ip *ip;
    struct icmp_echo *icmp;
    size_t reqsize;

    pData = pong->pData; /* to make slirp_state.h macro hackery work */

    nreplies = IcmpParseReplies(pong->buf, (DWORD)pong->bufsize);
    if (nreplies == 0)
    {
        DWORD error = GetLastError();
        if (error == IP_REQ_TIMED_OUT)
            Log2(("NAT: ping %p timed out\n", (void *)pong));
        else
            Log2(("NAT: ping %p: IcmpParseReplies: error %d\n",
                  (void *)pong, error));
        return;
    }

    reply = (ICMP_ECHO_REPLY *)pong->buf;

    if (reply->Status == IP_SUCCESS)
    {
        if (reply->Options.OptionsSize != 0) /* don't do options */
            return;

        /* need to remap &reply->Address ? */
        if (/* not a mapped loopback */ 1)
        {
            if (reply->Options.Ttl <= 1)
                return;
            --reply->Options.Ttl;
        }

        reqsize = reply->DataSize;
        if (   (reply->Options.Flags & IP_FLAG_DF) != 0
            && sizeof(struct ip) + sizeof(struct icmp_echo) + reqsize > if_mtu)
            return;

        m = icmpwin_get_mbuf(pData, reqsize);
        if (m == NULL)
            return;

        ip = mtod(m, struct ip *);
        icmp = (struct icmp_echo *)(mtod(m, char *) + sizeof(*ip));

        /* fill in ip (ip_output0() does the boilerplate for us) */
        ip->ip_tos = reply->Options.Tos;
        ip->ip_len = sizeof(*ip) + sizeof(*icmp) + reqsize;
        ip->ip_off = 0;
        ip->ip_ttl = reply->Options.Ttl;
        ip->ip_p = IPPROTO_ICMP;
        ip->ip_src.s_addr = reply->Address;
        ip->ip_dst = pong->reqiph.ip_src;

        icmp->icmp_type = ICMP_ECHOREPLY;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_echo_id = pong->reqicmph.icmp_echo_id;
        icmp->icmp_echo_seq = pong->reqicmph.icmp_echo_seq;

        m_append(pData, m, reqsize, reply->Data);

        icmp->icmp_cksum = in_cksum_skip(m, ip->ip_len, sizeof(*ip));
    }
    else {
        uint8_t type, code;

        switch (reply->Status) {
        case IP_DEST_NET_UNREACHABLE:
            type = ICMP_UNREACH; code = ICMP_UNREACH_NET;
            break;
        case IP_DEST_HOST_UNREACHABLE:
            type = ICMP_UNREACH; code = ICMP_UNREACH_HOST;
            break;
        case IP_DEST_PROT_UNREACHABLE:
            type = ICMP_UNREACH; code = ICMP_UNREACH_PROTOCOL;
            break;
        case IP_PACKET_TOO_BIG:
            type = ICMP_UNREACH; code = ICMP_UNREACH_NEEDFRAG;
            break;
        case IP_SOURCE_QUENCH:
            type = ICMP_SOURCEQUENCH; code = 0;
            break;
        case IP_TTL_EXPIRED_TRANSIT:
            type = ICMP_TIMXCEED; code = ICMP_TIMXCEED_INTRANS;
            break;
        case IP_TTL_EXPIRED_REASSEM:
            type = ICMP_TIMXCEED; code = ICMP_TIMXCEED_REASS;
            break;
        default:
            Log2(("NAT: ping reply status %d, dropped\n", reply->Status));
            return;
        }

        Log2(("NAT: ping status %d -> type %d/code %d\n",
              reply->Status, type, code));

        /*
         * XXX: we don't know the TTL of the request at the time this
         * ICMP error was generated (we can guess it was 1 for ttl
         * exceeded, but don't bother faking it).
         */
        m = icmpwin_get_error(pong, type, code);
        if (m == NULL)
            return;

        ip = mtod(m, struct ip *);

        ip->ip_tos = reply->Options.Tos;
        ip->ip_ttl = reply->Options.Ttl; /* XXX: decrement */
        ip->ip_src.s_addr = reply->Address;
    }

    Assert(ip->ip_len == m_length(m, NULL));
    ip_output(pData, NULL, m);
}


/*
 * Prepare mbuf with ICMP error type/code.
 * IP source must be filled by the caller.
 */
static struct mbuf *
icmpwin_get_error(struct pong *pong, int type, int code)
{
    PNATState pData = pong->pData;
    struct mbuf *m;
    struct ip *ip;
    struct icmp_echo *icmp;
    size_t reqsize;

    Log2(("NAT: ping error type %d/code %d\n", type, code));

    reqsize = sizeof(pong->reqiph) + sizeof(pong->reqicmph);

    m = icmpwin_get_mbuf(pData, reqsize);
    if (m == NULL)
        return NULL;

    ip = mtod(m, struct ip *);
    icmp = (struct icmp_echo *)(mtod(m, char *) + sizeof(*ip));

    ip->ip_tos = 0;
    ip->ip_len = sizeof(*ip) + sizeof(*icmp) + reqsize;
    ip->ip_off = 0;
    ip->ip_ttl = IPDEFTTL;
    ip->ip_p = IPPROTO_ICMP;
    ip->ip_src.s_addr = 0;      /* NB */
    ip->ip_dst = pong->reqiph.ip_src;

    icmp->icmp_type = type;
    icmp->icmp_code = code;
    icmp->icmp_cksum = 0;
    icmp->icmp_echo_id = 0;
    icmp->icmp_echo_seq = 0;

    m_append(pData, m, sizeof(pong->reqiph), (caddr_t)&pong->reqiph);
    m_append(pData, m, sizeof(pong->reqicmph), (caddr_t)&pong->reqicmph);

    icmp->icmp_cksum = in_cksum_skip(m, ip->ip_len, sizeof(*ip));

    return m;
}


/*
 * Replacing original simple slirp mbufs with real mbufs from freebsd
 * was a bit messy since assumption are different.  This leads to
 * rather ugly code at times.  Hide the gore here.
 */
static struct mbuf *
icmpwin_get_mbuf(PNATState pData, size_t reqsize)
{
    struct mbuf *m;

    reqsize += if_maxlinkhdr;
    reqsize += sizeof(struct ip) + sizeof(struct icmp_echo);

    if (reqsize <= MHLEN)
        /* good pings come in small packets */
        m = m_gethdr(pData, M_NOWAIT, MT_HEADER);
    else
        m = m_getjcl(pData, M_NOWAIT, MT_HEADER, M_PKTHDR, slirp_size(pData));

    if (m == NULL)
        return NULL;

    m->m_flags |= M_SKIP_FIREWALL;
    m->m_data += if_maxlinkhdr; /* reserve leading space for ethernet header */

    m->m_pkthdr.header = mtod(m, void *);
    m->m_len = sizeof(struct ip) + sizeof(struct icmp_echo);

    return m;
}
