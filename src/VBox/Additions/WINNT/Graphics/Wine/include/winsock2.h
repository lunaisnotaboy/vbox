/*
 * Winsock 2 definitions - used for ws2_32.dll
 *
 * Copyright (C) 1999 Ove Kaaven
 * Copyright (C) 2001 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME: Still missing required Winsock 2 definitions.
 */

/*
 * Sun LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Sun elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef _WINSOCK2API_
#define _WINSOCK2API_

/*
 * Setup phase
 */

/* Everything common between winsock.h and winsock2.h */
#ifndef INCL_WINSOCK_API_PROTOTYPES
#define INCL_WINSOCK_API_PROTOTYPES 1
#define WS_API_PROTOTYPES          1
#else
#define WS_API_PROTOTYPES          INCL_WINSOCK_API_PROTOTYPES
#endif

#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS  0
#define WS_API_TYPEDEFS            0
#else
#define WS_API_TYPEDEFS            INCL_WINSOCK_API_TYPEDEFS
#endif

#define __WINE_WINSOCK2__
#include <winsock.h>
#undef  __WINE_WINSOCK2__

#include <ws2def.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */


#ifndef USE_WS_PREFIX
#define SO_GROUP_ID                0x2001
#define SO_GROUP_PRIORITY          0x2002
#define SO_MAX_MSG_SIZE            0x2003
#define SO_PROTOCOL_INFOA          0x2004
#define SO_PROTOCOL_INFOW          0x2005
#define SO_PROTOCOL_INFO           WINELIB_NAME_AW(WS_SO_PROTOCOL_INFO)
#define PVD_CONFIG                 0x3001
#define SO_CONDITIONAL_ACCEPT      0x3002
#else
#define WS_SO_GROUP_ID             0x2001
#define WS_SO_GROUP_PRIORITY       0x2002
#define WS_SO_MAX_MSG_SIZE         0x2003
#define WS_SO_PROTOCOL_INFOA       0x2004
#define WS_SO_PROTOCOL_INFOW       0x2005
#define WS_SO_PROTOCOL_INFO        WINELIB_NAME_AW(WS_SO_PROTOCOL_INFO)
#define WS_PVD_CONFIG              0x3001
#define WS_SO_CONDITIONAL_ACCEPT   0x3002
#endif

/* protocol types */

#define FROM_PROTOCOL_INFO       (-1)

#ifndef USE_WS_PREFIX
#define SOCK_STREAM                1
#define SOCK_DGRAM                 2
#define SOCK_RAW                   3
#define SOCK_RDM                   4
#define SOCK_SEQPACKET             5
#else /* USE_WS_PREFIX */
#define WS_SOCK_STREAM             1
#define WS_SOCK_DGRAM              2
#define WS_SOCK_RAW                3
#define WS_SOCK_RDM                4
#define WS_SOCK_SEQPACKET          5
#endif /* USE_WS_PREFIX */

/* option flags per socket */

#define FD_MAX_EVENTS              10
#define FD_READ_BIT                0
#define FD_WRITE_BIT               1
#define FD_OOB_BIT                 2
#define FD_ACCEPT_BIT              3
#define FD_CONNECT_BIT             4
#define FD_CLOSE_BIT               5

/* Constants for LPCONDITIONPROC */
#define CF_ACCEPT                  0x0000
#define CF_REJECT                  0x0001
#define CF_DEFER                   0x0002

/* Constants for shutdown() */
#define SD_RECEIVE                 0x00
#define SD_SEND                    0x01
#define SD_BOTH                    0x02

/* Constants for WSAIoctl() */
#ifdef USE_WS_PREFIX
#define WS_IOC_UNIX                0x00000000
#define WS_IOC_WS2                 0x08000000
#define WS_IOC_PROTOCOL            0x10000000
#define WS_IOC_VENDOR              0x18000000
#define WS_IOC_VOID                0x20000000
#define WS_IOC_OUT                 0x40000000
#define WS_IOC_IN                  0x80000000
#define WS_IOC_INOUT               (WS_IOC_IN|WS_IOC_OUT)
#define _WSAIO(x,y)                (WS_IOC_VOID|(x)|(y))
#define _WSAIOR(x,y)               (WS_IOC_OUT|(x)|(y))
#define _WSAIOW(x,y)               (WS_IOC_IN|(x)|(y))
#define _WSAIORW(x,y)              (WS_IOC_INOUT|(x)|(y))
#define WS_SIO_ASSOCIATE_HANDLE               _WSAIOW(WS_IOC_WS2,1)
#define WS_SIO_ENABLE_CIRCULAR_QUEUEING       _WSAIO(WS_IOC_WS2,2)
#define WS_SIO_FIND_ROUTE                     _WSAIOR(WS_IOC_WS2,3)
#define WS_SIO_FLUSH                          _WSAIO(WS_IOC_WS2,4)
#define WS_SIO_GET_BROADCAST_ADDRESS          _WSAIOR(WS_IOC_WS2,5)
#define WS_SIO_GET_EXTENSION_FUNCTION_POINTER _WSAIORW(WS_IOC_WS2,6)
#define WS_SIO_GET_QOS                        _WSAIORW(WS_IOC_WS2,7)
#define WS_SIO_GET_GROUP_QOS                  _WSAIORW(WS_IOC_WS2,8)
#define WS_SIO_MULTIPOINT_LOOPBACK            _WSAIOW(WS_IOC_WS2,9)
#define WS_SIO_MULTICAST_SCOPE                _WSAIOW(WS_IOC_WS2,10)
#define WS_SIO_SET_QOS                        _WSAIOW(WS_IOC_WS2,11)
#define WS_SIO_SET_GROUP_QOS                  _WSAIOW(WS_IOC_WS2,12)
#define WS_SIO_TRANSLATE_HANDLE               _WSAIORW(WS_IOC_WS2,13)
#define WS_SIO_ROUTING_INTERFACE_QUERY        _WSAIORW(WS_IOC_WS2,20)
#define WS_SIO_ROUTING_INTERFACE_CHANGE       _WSAIOW(WS_IOC_WS2,21)
#define WS_SIO_ADDRESS_LIST_QUERY             _WSAIOR(WS_IOC_WS2,22)
#define WS_SIO_ADDRESS_LIST_CHANGE            _WSAIO(WS_IOC_WS2,23)
#define WS_SIO_QUERY_TARGET_PNP_HANDLE        _WSAIOR(WS_IOC_WS2,24)
#define WS_SIO_GET_INTERFACE_LIST             WS__IOR('t', 127, WS_u_long)
#else /* USE_WS_PREFIX */
#undef IOC_VOID
#undef IOC_IN
#undef IOC_OUT
#undef IOC_INOUT
#define IOC_UNIX                   0x00000000
#define IOC_WS2                    0x08000000
#define IOC_PROTOCOL               0x10000000
#define IOC_VENDOR                 0x18000000
#define IOC_VOID                   0x20000000
#define IOC_OUT                    0x40000000
#define IOC_IN                     0x80000000
#define IOC_INOUT                  (IOC_IN|IOC_OUT)
#define _WSAIO(x,y)                (IOC_VOID|(x)|(y))
#define _WSAIOR(x,y)               (IOC_OUT|(x)|(y))
#define _WSAIOW(x,y)               (IOC_IN|(x)|(y))
#define _WSAIORW(x,y)              (IOC_INOUT|(x)|(y))
#define SIO_ASSOCIATE_HANDLE       _WSAIOW(IOC_WS2,1)
#define SIO_ENABLE_CIRCULAR_QUEUEING _WSAIO(IOC_WS2,2)
#define SIO_FIND_ROUTE             _WSAIOR(IOC_WS2,3)
#define SIO_FLUSH                  _WSAIO(IOC_WS2,4)
#define SIO_GET_BROADCAST_ADDRESS  _WSAIOR(IOC_WS2,5)
#define SIO_GET_EXTENSION_FUNCTION_POINTER  _WSAIORW(IOC_WS2,6)
#define SIO_GET_QOS                _WSAIORW(IOC_WS2,7)
#define SIO_GET_GROUP_QOS          _WSAIORW(IOC_WS2,8)
#define SIO_MULTIPOINT_LOOPBACK    _WSAIOW(IOC_WS2,9)
#define SIO_MULTICAST_SCOPE        _WSAIOW(IOC_WS2,10)
#define SIO_SET_QOS                _WSAIOW(IOC_WS2,11)
#define SIO_SET_GROUP_QOS          _WSAIOW(IOC_WS2,12)
#define SIO_TRANSLATE_HANDLE       _WSAIORW(IOC_WS2,13)
#define SIO_ROUTING_INTERFACE_QUERY _WSAIORW(IOC_WS2,20)
#define SIO_ROUTING_INTERFACE_CHANGE _WSAIOW(IOC_WS2,21)
#define SIO_ADDRESS_LIST_QUERY     _WSAIOR(IOC_WS2,22)
#define SIO_ADDRESS_LIST_CHANGE    _WSAIO(IOC_WS2,23)
#define SIO_QUERY_TARGET_PNP_HANDLE _WSAIOR(IOC_WS2,24)
#define SIO_GET_INTERFACE_LIST     _IOR ('t', 127, u_long)
#endif /* USE_WS_PREFIX */

/* Constants for WSAIoctl() */
#define WSA_FLAG_OVERLAPPED        0x01
#define WSA_FLAG_MULTIPOINT_C_ROOT 0x02
#define WSA_FLAG_MULTIPOINT_C_LEAF 0x04
#define WSA_FLAG_MULTIPOINT_D_ROOT 0x08
#define WSA_FLAG_MULTIPOINT_D_LEAF 0x10

/* Constants for WSAJoinLeaf() */
#define JL_SENDER_ONLY    0x01
#define JL_RECEIVER_ONLY  0x02
#define JL_BOTH           0x04


#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#define MAX_PROTOCOL_CHAIN         7
#define BASE_PROTOCOL              1
#define LAYERED_PROTOCOL           0

typedef struct _WSAPROTOCOLCHAIN
{
    int ChainLen;                  /* the length of the chain,     */
                                   /* length = 0 means layered protocol, */
                                   /* length = 1 means base protocol, */
                                   /* length > 1 means protocol chain */
    DWORD ChainEntries[MAX_PROTOCOL_CHAIN]; /* a list of dwCatalogEntryIds */
} WSAPROTOCOLCHAIN, * LPWSAPROTOCOLCHAIN;

#define XP1_CONNECTIONLESS                  0x00000001
#define XP1_GUARANTEED_DELIVERY             0x00000002
#define XP1_GUARANTEED_ORDER                0x00000004
#define XP1_MESSAGE_ORIENTED                0x00000008
#define XP1_PSEUDO_STREAM                   0x00000010
#define XP1_GRACEFUL_CLOSE                  0x00000020
#define XP1_EXPEDITED_DATA                  0x00000040
#define XP1_CONNECT_DATA                    0x00000080
#define XP1_DISCONNECT_DATA                 0x00000100
#define XP1_SUPPORT_BROADCAST               0x00000200
#define XP1_SUPPORT_MULTIPOINT              0x00000400
#define XP1_MULTIPOINT_CONTROL_PLANE        0x00000800
#define XP1_MULTIPOINT_DATA_PLANE           0x00001000
#define XP1_QOS_SUPPORTED                   0x00002000
#define XP1_INTERRUPT                       0x00004000
#define XP1_UNI_SEND                        0x00008000
#define XP1_UNI_RECV                        0x00010000
#define XP1_IFS_HANDLES                     0x00020000
#define XP1_PARTIAL_MESSAGE                 0x00040000

#define BIGENDIAN                           0x0000
#define LITTLEENDIAN                        0x0001

#define SECURITY_PROTOCOL_NONE              0x0000

#define WSAPROTOCOL_LEN  255
typedef struct _WSAPROTOCOL_INFOA
{
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    int iVersion;
    int iAddressFamily;
    int iMaxSockAddr;
    int iMinSockAddr;
    int iSocketType;
    int iProtocol;
    int iProtocolMaxOffset;
    int iNetworkByteOrder;
    int iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    CHAR szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOA, * LPWSAPROTOCOL_INFOA;

typedef struct _WSAPROTOCOL_INFOW
{
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    int iVersion;
    int iAddressFamily;
    int iMaxSockAddr;
    int iMinSockAddr;
    int iSocketType;
    int iProtocol;
    int iProtocolMaxOffset;
    int iNetworkByteOrder;
    int iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    WCHAR szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOW, *LPWSAPROTOCOL_INFOW;

DECL_WINELIB_TYPE_AW(WSAPROTOCOL_INFO)
DECL_WINELIB_TYPE_AW(LPWSAPROTOCOL_INFO)

typedef struct _WSANETWORKEVENTS
{
    LONG lNetworkEvents;
    int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, *LPWSANETWORKEVENTS;

typedef struct _WSANSClassInfoA
{
    LPSTR lpszName;
    DWORD dwNameSpace;
    DWORD dwValueType;
    DWORD dwValueSize;
    LPVOID lpValue;
} WSANSCLASSINFOA, *PWSANSCLASSINFOA, *LPWSANSCLASSINFOA;

typedef struct _WSANSClassInfoW
{
    LPSTR lpszName;
    DWORD dwNameSpace;
    DWORD dwValueType;
    DWORD dwValueSize;
    LPVOID lpValue;
} WSANSCLASSINFOW, *PWSANSCLASSINFOW, *LPWSANSCLASSINFOW;

DECL_WINELIB_TYPE_AW(WSANSCLASSINFO)
DECL_WINELIB_TYPE_AW(PWSANSCLASSINFO)
DECL_WINELIB_TYPE_AW(LPWSANSCLASSINFO)

typedef struct _WSAServiceClassInfoA
{
    LPGUID lpServiceClassId;
    LPSTR lpszServiceClassName;
    DWORD dwCount;
    LPWSANSCLASSINFOA lpClassInfos;
} WSASERVICECLASSINFOA, *PWSASERVICECLASSINFOA, *LPWSASERVICECLASSINFOA;

typedef struct _WSAServiceClassInfoW
{
    LPGUID lpServiceClassId;
    LPWSTR lpszServiceClassName;
    DWORD dwCount;
    LPWSANSCLASSINFOW lpClassInfos;
} WSASERVICECLASSINFOW, *PWSASERVICECLASSINFOW, *LPWSASERVICECLASSINFOW;


DECL_WINELIB_TYPE_AW(WSASERVICECLASSINFO)
DECL_WINELIB_TYPE_AW(PWSASERVICECLASSINFO)
DECL_WINELIB_TYPE_AW(LPWSASERVICECLASSINFO)

typedef struct _WSABUF
{
    ULONG len;
    CHAR* buf;
} WSABUF, *LPWSABUF;

#define WSAEVENT      HANDLE
#define LPWSAEVENT    LPHANDLE
#define WSAOVERLAPPED OVERLAPPED
typedef struct _OVERLAPPED* LPWSAOVERLAPPED;

#define WSA_IO_PENDING             (ERROR_IO_PENDING)
#define WSA_IO_INCOMPLETE          (ERROR_IO_INCOMPLETE)
#define WSA_INVALID_HANDLE         (ERROR_INVALID_HANDLE)
#define WSA_INVALID_PARAMETER      (ERROR_INVALID_PARAMETER)
#define WSA_NOT_ENOUGH_MEMORY      (ERROR_NOT_ENOUGH_MEMORY)
#define WSA_OPERATION_ABORTED      (ERROR_OPERATION_ABORTED)

#define WSA_INVALID_EVENT          ((WSAEVENT)NULL)
#define WSA_MAXIMUM_WAIT_EVENTS    (MAXIMUM_WAIT_OBJECTS)
#define WSA_WAIT_FAILED            ((DWORD)-1L)
#define WSA_WAIT_EVENT_0           (WAIT_OBJECT_0)
#define WSA_WAIT_IO_COMPLETION     (WAIT_IO_COMPLETION)
#define WSA_WAIT_TIMEOUT           (WAIT_TIMEOUT)
#define WSA_INFINITE               (INFINITE)

typedef unsigned int   GROUP;
#define SG_UNCONSTRAINED_GROUP   0x01
#define SG_CONSTRAINED_GROUP     0x02

/*
 * FLOWSPEC and SERVICETYPE should eventually move to qos.h
 */

typedef ULONG   SERVICETYPE;

typedef struct _FLOWSPEC {
       unsigned int      TokenRate;
       unsigned int      TokenBucketSize;
       unsigned int      PeakBandwidth;
       unsigned int      Latency;
       unsigned int      DelayVariation;
       SERVICETYPE       ServiceType;
       unsigned int      MaxSduSize;
       unsigned int      MinimumPolicedSize;
   } FLOWSPEC, *PFLOWSPEC, *LPFLOWSPEC;

typedef struct _QUALITYOFSERVICE {
        FLOWSPEC           SendingFlowspec;
        FLOWSPEC           ReceivingFlowspec;
        WSABUF             ProviderSpecific;
   } QOS, *LPQOS;

typedef int (CALLBACK *LPCONDITIONPROC)
(
    LPWSABUF lpCallerId,
    LPWSABUF lpCallerData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    LPWSABUF lpCalleeId,
    LPWSABUF lpCalleeData,
    GROUP *g,
    DWORD dwCallbackData
);

typedef void (CALLBACK *LPWSAOVERLAPPED_COMPLETION_ROUTINE)
(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
);

#ifndef _tagBLOB_DEFINED
#define _tagBLOB_DEFINED
#define _BLOB_DEFINED
#define _LPBLOB_DEFINED
typedef struct _BLOB {
        ULONG   cbSize;
        BYTE   *pBlobData;
} BLOB, *LPBLOB;
#endif

/*   addressfamily protocol pairs */
typedef struct _AFPROTOCOLS {
        INT     iAddressFamily;
        INT     iProtocol;
} AFPROTOCOLS, *PAFPROTOCOLS, *LPAFPROTOCOLS;

/* client query definitions */
typedef enum _WSAEcomparator {
        COMP_EQUAL = 0,
        COMP_NOTLESS
} WSAECOMPARATOR, *PWSAECOMPARATOR, *LPWSAECOMPARATOR;

typedef struct _WSAVersion {
        DWORD           dwVersion;
        WSAECOMPARATOR  ecHow;
} WSAVERSION, *PWSAVERSION, *LPWSAVERSION;


typedef struct _WSAQuerySetA {
        DWORD           dwSize;
        LPSTR           lpszServiceInstanceName;
        LPGUID          lpServiceClassId;
        LPWSAVERSION    lpVersion;
        LPSTR           lpszComment;
        DWORD           dwNameSpace;
        LPGUID          lpNSProviderId;
        LPSTR           lpszContext;
        DWORD           dwNumberOfProtocols;
        LPAFPROTOCOLS   lpafpProtocols;
        LPSTR           lpszQueryString;
        DWORD           dwNumberOfCsAddrs;
        LPCSADDR_INFO   lpcsaBuffer;
        DWORD           dwOutputFlags;
        LPBLOB          lpBlob;
} WSAQUERYSETA, *PWSAQUERYSETA, *LPWSAQUERYSETA;

typedef struct _WSAQuerySetW {
        DWORD           dwSize;
        LPWSTR          lpszServiceInstanceName;
        LPGUID          lpServiceClassId;
        LPWSAVERSION    lpVersion;
        LPWSTR          lpszComment;
        DWORD           dwNameSpace;
        LPGUID          lpNSProviderId;
        LPWSTR          lpszContext;
        DWORD           dwNumberOfProtocols;
        LPAFPROTOCOLS   lpafpProtocols;
        LPWSTR          lpszQueryString;
        DWORD           dwNumberOfCsAddrs;
        LPCSADDR_INFO   lpcsaBuffer;
        DWORD           dwOutputFlags;
        LPBLOB          lpBlob;
} WSAQUERYSETW, *PWSAQUERYSETW, *LPWSAQUERYSETW;

DECL_WINELIB_TYPE_AW(WSAQUERYSET)
DECL_WINELIB_TYPE_AW(PWSAQUERYSET)
DECL_WINELIB_TYPE_AW(LPWSAQUERYSET)

typedef enum _WSAESETSERVICEOP {
        RNRSERVICE_REGISTER = 0,
        RNRSERVICE_DEREGISTER,
        RNRSERVICE_DELETE
} WSAESETSERVICEOP, *PWSAESETSERVICEOP, *LPWSAESETSERVICEOP;

typedef struct _WSANAMESPACE_INFOA {
        GUID    NSProviderId;
        DWORD   dwNameSpace;
        BOOL    fActive;
        DWORD   dwVersion;
        LPSTR   lpszIdentifier;
} WSANAMESPACE_INFOA, *PWSANAMESPACE_INFOA, *LPWSANAMESPACE_INFOA;

typedef struct _WSANAMESPACE_INFOW {
        GUID    NSProviderId;
        DWORD   dwNameSpace;
        BOOL    fActive;
        DWORD   dwVersion;
        LPWSTR  lpszIdentifier;
} WSANAMESPACE_INFOW, *PWSANAMESPACE_INFOW, *LPWSANAMESPACE_INFOW;

DECL_WINELIB_TYPE_AW(WSANAMESPACE_INFO)
DECL_WINELIB_TYPE_AW(PWSANAMESPACE_INFO)
DECL_WINELIB_TYPE_AW(LPWSANAMESPACE_INFO)

/*
 * Winsock Function Typedefs
 *
 * Remember to keep this section in sync with the
 * "Prototypes" section in winsock.h.
 */
#if WS_API_TYPEDEFS
typedef HANDLE (WINAPI *LPFN_WSAASYNCGETHOSTBYADDR)(HWND,WS(u_int),const char*,int,int,char*,int);
typedef HANDLE (WINAPI *LPFN_WSAASYNCGETHOSTBYNAME)(HWND,WS(u_int),const char*,char*,int);
typedef HANDLE (WINAPI *LPFN_WSAASYNCGETPROTOBYNAME)(HWND,WS(u_int),const char*,char*,int);
typedef HANDLE (WINAPI *LPFN_WSAASYNCGETPROTOBYNUMBER)(HWND,WS(u_int),int,char*,int);
typedef HANDLE (WINAPI *LPFN_WSAASYNCGETSERVBYNAME)(HWND,WS(u_int),const char*,const char*,char*,int);
typedef HANDLE (WINAPI *LPFN_WSAASYNCGETSERVBYPORT)(HWND,WS(u_int),int,const char*,char*,int);
typedef int (WINAPI *LPFN_WSAASYNCSELECT)(SOCKET,HWND,WS(u_int),LONG);
typedef int (WINAPI *LPFN_WSACANCELASYNCREQUEST)(HANDLE);
typedef int (WINAPI *LPFN_WSACANCELBLOCKINGCALL)(void);
typedef int (WINAPI *LPFN_WSACLEANUP)(void);
typedef int (WINAPI *LPFN_WSAGETLASTERROR)(void);
typedef BOOL (WINAPI *LPFN_WSAISBLOCKING)(void);
typedef FARPROC (WINAPI *LPFN_WSASETBLOCKINGHOOK)(FARPROC);
typedef void (WINAPI *LPFN_WSASETLASTERROR)(int);
typedef int (WINAPI *LPFN_WSASTARTUP)(WORD,LPWSADATA);
typedef int (WINAPI *LPFN_WSAUNHOOKBLOCKINGHOOK)(void);

typedef SOCKET (WINAPI *LPFN_ACCEPT)(SOCKET,struct WS(sockaddr)*,int*);
typedef int (WINAPI *LPFN_BIND)(SOCKET,const struct WS(sockaddr)*,int);
typedef int (WINAPI *LPFN_CLOSESOCKET)(SOCKET);
typedef int (WINAPI *LPFN_CONNECT)(SOCKET,const struct WS(sockaddr)*,int);
typedef struct WS(hostent)* (WINAPI *LPFN_GETHOSTBYADDR)(const char*,int,int);
typedef struct WS(hostent)* (WINAPI *LPFN_GETHOSTBYNAME)(const char*);
typedef int (WINAPI *LPFN_GETHOSTNAME)(char*,int);
typedef int (WINAPI *LPFN_GETPEERNAME)(SOCKET,struct WS(sockaddr)*,int*);
typedef struct WS(protoent)* (WINAPI *LPFN_GETPROTOBYNAME)(const char*);
typedef struct WS(protoent)* (WINAPI *LPFN_GETPROTOBYNUMBER)(int);
#ifdef WS_DEFINE_SELECT
typedef int (WINAPI* LPFN_SELECT)(int,WS(fd_set)*,WS(fd_set)*,WS(fd_set)*,const struct WS(timeval)*);
#endif
typedef struct WS(servent)* (WINAPI *LPFN_GETSERVBYNAME)(const char*,const char*);
typedef struct WS(servent)* (WINAPI *LPFN_GETSERVBYPORT)(int,const char*);
typedef int (WINAPI *LPFN_GETSOCKNAME)(SOCKET,struct WS(sockaddr)*,int*);
typedef int (WINAPI *LPFN_GETSOCKOPT)(SOCKET,int,int,char*,int*);
typedef WS(u_long) (WINAPI *LPFN_HTONL)(WS(u_long));
typedef WS(u_short) (WINAPI *LPFN_HTONS)(WS(u_short));
typedef ULONG (WINAPI *LPFN_INET_ADDR)(const char*);
typedef char* (WINAPI *LPFN_INET_NTOA)(struct WS(in_addr);
typedef int (WINAPI *LPFN_IOCTLSOCKET)(SOCKET,LONG,WS(u_long)*);
typedef int (WINAPI *LPFN_LISTEN)(SOCKET,int);
typedef WS(u_long) (WINAPI *LPFN_NTOHL)(WS(u_long));
typedef WS(u_short) (WINAPI *LPFN_NTOHS)(WS(u_short));
typedef int (WINAPI *LPFN_RECV)(SOCKET,char*,int,int);
typedef int (WINAPI *LPFN_RECVFROM)(SOCKET,char*,int,int,struct WS(sockaddr)*,int*);
typedef int (WINAPI *LPFN_SEND)(SOCKET,const char*,int,int);
typedef int (WINAPI *LPFN_SENDTO)(SOCKET,const char*,int,int,const struct WS(sockaddr)*,int);
typedef int (WINAPI *LPFN_SETSOCKOPT)(SOCKET,int,int,const char*,int);
typedef int (WINAPI *LPFN_SHUTDOWN)(SOCKET,int);
typedef SOCKET (WINAPI *LPFN_SOCKET)(int,int,int);
#endif /* WS_API_TYPEDEFS */



/*
 * Winsock2 Prototypes
 *
 * Remember to keep this section in sync with the
 * "Winsock2 Function Typedefs" section below.
 */
#if WS_API_PROTOTYPES
SOCKET WINAPI WSAAccept(SOCKET,struct WS(sockaddr)*,LPINT,LPCONDITIONPROC,DWORD);
INT WINAPI WSAAddressToStringA(LPSOCKADDR,DWORD,LPWSAPROTOCOL_INFOA,LPSTR,LPDWORD);
INT WINAPI WSAAddressToStringW(LPSOCKADDR,DWORD,LPWSAPROTOCOL_INFOW,LPWSTR,LPDWORD);
#define WSAAddressToString         WINELIB_NAME_AW(WSAAddressToString)
BOOL WINAPI WSACloseEvent(WSAEVENT);
int WINAPI WSAConnect(SOCKET,const struct WS(sockaddr)*,int,LPWSABUF,LPWSABUF,LPQOS,LPQOS);
WSAEVENT WINAPI WSACreateEvent(void);
INT WINAPI WSADuplicateSocketA(SOCKET,DWORD,LPWSAPROTOCOL_INFOA);
INT WINAPI WSADuplicateSocketW(SOCKET,DWORD,LPWSAPROTOCOL_INFOW);
#define WSADuplicateSocket         WINELIB_NAME_AW(WSADuplicateSocket)
INT WINAPI WSAEnumNameSpaceProvidersA(LPDWORD,LPWSANAMESPACE_INFOA);
INT WINAPI WSAEnumNameSpaceProvidersW(LPDWORD,LPWSANAMESPACE_INFOW);
#define WSAEnumNameSpaceProviders  WINELIB_NAME_AW(WSAEnumNameSpaceProviders)
int WINAPI WSAEnumNetworkEvents(SOCKET,WSAEVENT,LPWSANETWORKEVENTS);
int WINAPI WSAEnumProtocolsA(LPINT,LPWSAPROTOCOL_INFOA,LPDWORD);
int WINAPI WSAEnumProtocolsW(LPINT,LPWSAPROTOCOL_INFOW,LPDWORD);
#define WSAEnumProtocols           WINELIB_NAME_AW(WSAEnumProtocols)
int WINAPI WSAEventSelect(SOCKET,WSAEVENT,LONG);
BOOL WINAPI WSAGetOverlappedResult(SOCKET,LPWSAOVERLAPPED,LPDWORD,BOOL,LPDWORD);
BOOL WINAPI WSAGetQOSByName(SOCKET,LPWSABUF,LPQOS);
INT WINAPI WSAGetServiceClassInfoA(LPGUID,LPGUID,LPDWORD,LPWSASERVICECLASSINFOA);
INT WINAPI WSAGetServiceClassInfoW(LPGUID,LPGUID,LPDWORD,LPWSASERVICECLASSINFOW);
#define WSAGetServiceClassInfo     WINELIB_NAME_AW(WSAGetServiceClassInfo)
INT WINAPI WSAGetServiceClassNameByClassIdA(LPGUID,LPSTR,LPDWORD);
INT WINAPI WSAGetServiceClassNameByClassIdW(LPGUID,LPWSTR,LPDWORD);
#define WSAGetServiceClassNameByClassId WINELIB_NAME_AW(WSAGetServiceClassNameByClassId)
int WINAPI WSAHtonl(SOCKET,WS(u_long),WS(u_long)*);
int WINAPI WSAHtons(SOCKET,WS(u_short),WS(u_short)*);
int WINAPI WSAInstallServiceClassA(LPWSASERVICECLASSINFOA);
int WINAPI WSAInstallServiceClassW(LPWSASERVICECLASSINFOW);
#define WSAInstallServiceClass     WINELIB_NAME_AW(WSAInstallServiceClass)
int WINAPI WSAIoctl(SOCKET,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
SOCKET WINAPI WSAJoinLeaf(SOCKET,const struct WS(sockaddr)*,int,LPWSABUF,LPWSABUF,LPQOS,LPQOS,DWORD);
INT WINAPI WSALookupServiceBeginA(LPWSAQUERYSETA,DWORD,LPHANDLE);
INT WINAPI WSALookupServiceBeginW(LPWSAQUERYSETW,DWORD,LPHANDLE);
#define WSALookupServiceBegin      WINELIB_NAME_AW(WSALookupServiceBegin)
INT WINAPI WSALookupServiceEnd(HANDLE);
INT WINAPI WSALookupServiceNextA(HANDLE,DWORD,LPDWORD,LPWSAQUERYSETA);
INT WINAPI WSALookupServiceNextW(HANDLE,DWORD,LPDWORD,LPWSAQUERYSETW);
#define WSALookupServiceNext       WINELIB_NAME_AW(WSALookupServiceNext) 
int WINAPI WSANtohl(SOCKET,WS(u_long),WS(u_long)*);
int WINAPI WSANtohs(SOCKET,WS(u_short),WS(u_short)*);
INT WINAPI WSAProviderConfigChange(LPHANDLE,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int WINAPI WSARecv(SOCKET,LPWSABUF,DWORD,LPDWORD,LPDWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int WINAPI WSARecvDisconnect(SOCKET,LPWSABUF);
int WINAPI WSARecvFrom(SOCKET,LPWSABUF,DWORD,LPDWORD,LPDWORD,struct WS(sockaddr)*,LPINT,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
INT WINAPI WSARemoveServiceClass(LPGUID);
BOOL WINAPI WSAResetEvent(WSAEVENT);
int WINAPI WSASend(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int WINAPI WSASendDisconnect(SOCKET,LPWSABUF);
int WINAPI WSASendTo(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,const struct WS(sockaddr)*,int,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
BOOL WINAPI WSASetEvent(WSAEVENT);
INT WINAPI WSASetServiceA(LPWSAQUERYSETA,WSAESETSERVICEOP,DWORD);
INT WINAPI WSASetServiceW(LPWSAQUERYSETW,WSAESETSERVICEOP,DWORD);
#define WSASetService   WINELIB_NAME_AW(WSASetService)
SOCKET WINAPI WSASocketA(int,int,int,LPWSAPROTOCOL_INFOA,GROUP,DWORD);
SOCKET WINAPI WSASocketW(int,int,int,LPWSAPROTOCOL_INFOW,GROUP,DWORD);
#define WSASocket WINELIB_NAME_AW(WSASocket)
INT WINAPI WSAStringToAddressA(LPSTR,INT,LPWSAPROTOCOL_INFOA,LPSOCKADDR,LPINT);
INT WINAPI WSAStringToAddressW(LPWSTR,INT,LPWSAPROTOCOL_INFOW,LPSOCKADDR,LPINT);
#define WSAStringToAddress WINELIB_NAME_AW(WSAStringToAddress)
DWORD WINAPI WSAWaitForMultipleEvents(DWORD,const WSAEVENT*,BOOL,DWORD,BOOL);
#endif /* WS_API_PROTOTYPES */



/*
 * Winsock2 Function Typedefs
 *
 * Remember to keep this section in sync with the
 * "Winsock2 Prototypes" section above.
 */
#if WS_API_TYPEDEFS
typedef SOCKET (WINAPI *LPFN_WSAACCEPT)(SOCKET,WS(sockaddr)*,LPINT,LPCONDITIONPROC,DWORD);
typedef INT (WINAPI *LPFN_WSAADRESSTOSTRINGA)(LPSOCKADDR,DWORD,LPWSAPROTOCOL_INFOA,LPSTR,LPDWORD);
typedef INT (WINAPI *LPFN_WSAADRESSTOSTRINGW)(LPSOCKADDR,DWORD,LPWSAPROTOCOL_INFOW,LPWSTR,LPDWORD);
#define LPFN_WSAADDRESSTOSTRING    WINELIB_NAME_AW(LPFN_WSAADDRESSTOSTRING)
typedef BOOL (WINAPI *LPFN_WSACLOSEEVENT)(WSAEVENT);
typedef int (WINAPI *LPFN_WSACONNECT)(SOCKET,const struct WS(sockaddr)*,int,LPWSABUF,LPWSABUF,LPQOS,LPQOS);
typedef WSAEVENT (WINAPI *LPFN_WSACREATEEVENT)(void);
typedef INT (WINAPI *LPFN_WSADUPLICATESOCKETA)(SOCKET,DWORD,LPWSAPROTOCOL_INFOA);
typedef INT (WINAPI *LPFN_WSADUPLICATESOCKETW)(SOCKET,DWORD,LPWSAPROTOCOL_INFOW);
#define LPFN_WSADUPLICATESOCKET    WINELIB_NAME_AW(LPFN_WSADUPLICATESOCKET)
typedef INT (WINAPI *LPFNWSAENUMNAMESPACEPROVIDERSA)(LPDWORD,LPWSANAMESPACE_INFOA);
typedef INT (WINAPI *LPFNWSAENUMNAMESPACEPROVIDERSW)(LPDWORD,LPWSANAMESPACE_INFOW);
#define LPFN_WSAENUMNAMESPACEPROVIDERS WINELIB_NAME_AW(LPFNWSAENUMNAMESPACEPROVIDERS)
typedef int (WINAPI *LPFN_WSAENUMNETWORKEVENT)(SOCKET,WSAEVENT,LPWSANETWORKEVENTS);
typedef int (WINAPI *LPFN_WSAENUMPROTOCOLSA)(LPINT,LPWSAPROTOCOL_INFOA,LPDWORD);
typedef int (WINAPI *LPFN_WSAENUMPROTOCOLSW)(LPINT,LPWSAPROTOCOL_INFOW,LPDWORD);
#define LPFN_WSAENUMPROTOCOLS      WINELIB_NAME_AW(LPFN_WSAENUMPROTOCOLS)
typedef int (WINAPI *LPFN_WSAEVENTSELECT)(SOCKET,WSAEVENT,LONG);
typedef BOOL (WINAPI *LPFN_WSAGETOVERLAPPEDRESULT)(SOCKET,LPWSAOVERLAPPED,LPDWORD,BOOL,LPDWORD);
typedef BOOL (WINAPI *LPFNWSAGETQOSBYNAME)(SOCKET,LPWSABUF,LPQOS);
typedef INT (WINAPI *LPFNWSAGETSERVICECLASSINFOA)(LPGUID,LPGUID,LPDWORD,LPWSASERVICECLASSINFOA);
typedef INT (WINAPI *LPFNWSAGETSERVICECLASSINFOW)(LPGUID,LPGUID,LPDWORD,LPWSASERVICECLASSINFOW);
#define LPFN_LPFNWSAGETSERVICECLASSINFO WINELIB_NAME_AW(LPFN_LPFNWSAGETSERVICECLASSINFO)
typedef INT (WINAPI *LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDA)(LPGUID,LPSTR,LPDWORD);
typedef INT (WINAPI *LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDW)(LPGUID,LPWSTR,LPDWORD);
#define LPFN_WSAGETSERVICECLASSNAMEBYCLASSID WINELIB_NAME_AW(LPFN_WSAGETSERVICECLASSNAMEBYCLASSID)
typedef int (WINAPI *LPFN_WSAHTONL)(SOCKET,WS(u_long),WS(u_long)*);
typedef int (WINAPI *LPFN_WSAHTONS)(SOCKET,WS(u_short),WS(u_short)*);
typedef int (WINAPI LPFN_WSAINSTALLSERVICECLASSA)(LPWSASERVICECLASSINFOA);
typedef int (WINAPI LPFN_WSAINSTALLSERVICECLASSW)(LPWSASERVICECLASSINFOW);
typedef int (WINAPI *LPFN_WSAIOCTL)(SOCKET,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI *LPFN_WSAJOINLEAF)(SOCKET,const struct WS(sockaddr)*,int,LPWSABUF,LPWSABUF,LPQOS,LPQOS,DWORD);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICEBEGINA)(LPWSAQUERYSETA,DWORD,LPHANDLE);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICEBEGINW)(LPWSAQUERYSETW,DWORD,LPHANDLE);
#define LPFN_WSALOOKUPSERVICEBEGIN WINELIB_NAME_AW(LPFN_WSALOOKUPSERVICEBEGIN);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICEEND(HANDLE);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICENEXTA)(HANDLE,DWORD,LPDWORD,LPWSAQUERYSETA);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICENEXTW)(HANDLE,DWORD,LPDWORD,LPWSAQUERYSETW);
#define LPFN_WSALOOKUPSERVICENEXT WINELIB_NAME_AW(LPFN_WSALOOKUPSERVICENEXT)
typedef int (WINAPI *LPFN_WSANTOHL)(SOCKET,WS(u_long),WS(u_long)*);
typedef int (WINAPI *LPFN_WSANTOHS)(SOCKET,WS(u_short),WS(u_short)*);
typedef INT (WINAPI *LPFN_WSAPROVIDERCONFIGCHANGE)(LPHANDLE,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI *LPFN_WSARECV)(SOCKET,LPWSABUF,DWORD,LPDWORD,LPDWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI *LPFN_WSARECVDISCONNECT)(SOCKET,LPWSABUF);
typedef int (WINAPI *LPFN_WSARECVFROM)(SOCKET,LPWSABUF,DWORD,LPDWORD,LPDWORD,struct WS(sockaddr)*,LPINT,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef INT (WINAPI *LPFN_WSAREMOVESERVICECLASS)(LPGUID);
typedef BOOL (WINAPI *LPFN_WSARESETEVENT)(WSAEVENT);
typedef int (WINAPI *LPFN_WSASEND)(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI *LPFN_WSASENDDISCONNECT)(SOCKET,LPWSABUF);
typedef int (WINAPI *LPFN_WSASENDTO)(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,const struct WS(sockaddr)*,int,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef BOOL (WINAPI *LPFN_WSASETEVENT)(WSAEVENT);
typedef INT (WINAPI *LPFN_WSASETSERVICEA)(LPWSAQUERYSETA,WSAESETSERVICEOP,DWORD);
typedef INT (WINAPI *LPFN_WSASETSERVICEW)(LPWSAQUERYSETW,WSAESETSERVICEOP,DWORD);
#define LPFN_WSASETSERVICE WINELIB_NAME_AW(LPFN_WSASETSERVICE)
typedef SOCKET (WINAPI *LPFN_WSASOCKETA)(int,int,int,LPWSAPROTOCOL_INFOA,GROUP,DWORD);
typedef SOCKET (WINAPI *LPFN_WSASOCKETW)(int,int,int,LPWSAPROTOCOL_INFOW,GROUP,DWORD);
typedef INT (WINAPI *LPFN_WSASTRINGTOADDRESSA)(LPSTR,INT,LPWSAPROTOCOL_INFOA,LPSOCKADDR,LPINT);
typedef INT (WINAPI *LPFN_WSASTRINGTOADDRESSW)(LPSTR,INT,LPWSAPROTOCOL_INFOA,LPSOCKADDR,LPINT);
#define LPFN_WSASOCKET             WINELIB_NAME_AW(LPFN_WSASOCKET)
#define LPFN_WSASTRINGTOADDRESS    WINELIB_NAME_AW(LPFN_WSASTRINGTOADDRESS)
typedef DWORD (WINAPI *LPFN_WSAWAITFORMULTIPLEEVENTS)(DWORD,const WSAEVENT*,BOOL,DWORD,BOOL);
#endif /* WS_API_TYPEDEFS */


/* Condition function return values */
#define CF_ACCEPT       0x0000
#define CF_REJECT       0x0001
#define CF_DEFER        0x0002

#ifdef __cplusplus
}
#endif

#undef WS
#undef WS_API_PROTOTYPES
#undef WS_API_TYPEDEFS

#endif /* __WINSOCK2API__ */
