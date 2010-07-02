/*
 * vboxweb.h:
 *      header file for "real" web server code.
 *
 * Copyright (C) 2006-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/****************************************************************************
 *
 * debug macro
 *
 ****************************************************************************/

void WebLog(const char *pszFormat, ...);

#define WEBDEBUG(a) if (g_fVerbose) { WebLog a; }

#ifdef DEBUG
#define LOG_GROUP LOG_GROUP_WEBSERVICE
#include <VBox/log.h>
#endif

#include <VBox/com/VirtualBox.h>
#include <VBox/com/Guid.h>
#include <VBox/com/AutoLock.h>

#include <VBox/err.h>

#include <iprt/stream.h>

#include <string>

/****************************************************************************
 *
 * global variables
 *
 ****************************************************************************/

extern ComPtr<IVirtualBox> g_pVirtualBox;
extern bool g_fVerbose;

extern PRTSTREAM g_pstrLog;

extern util::WriteLockHandle  *g_pAuthLibLockHandle;
extern util::WriteLockHandle  *g_pSessionsLockHandle;

/****************************************************************************
 *
 * typedefs
 *
 ****************************************************************************/

// type used by gSOAP-generated code
typedef std::string WSDLT_ID;               // combined managed object ref (session ID plus object ID)
typedef std::string vbox__uuid;

/****************************************************************************
 *
 * SOAP exceptions
 *
 ****************************************************************************/

void RaiseSoapInvalidObjectFault(struct soap *soap, WSDLT_ID obj);

void RaiseSoapRuntimeFault(struct soap *soap, HRESULT apirc, IUnknown *pObj);

/****************************************************************************
 *
 * conversion helpers
 *
 ****************************************************************************/

std::string ConvertComString(const com::Bstr &bstr);

std::string ConvertComString(const com::Guid &bstr);

/****************************************************************************
 *
 * managed object reference classes
 *
 ****************************************************************************/

class WebServiceSessionPrivate;
class ManagedObjectRef;

/**
 *  An instance of this gets created for every client that logs onto the
 *  webservice (via the special IWebsessionManager::logon() SOAP API) and
 *  maintains the managed object references for that session.
 */
class WebServiceSession
{
    friend class ManagedObjectRef;

    private:
        uint64_t                    _uSessionID;
        WebServiceSessionPrivate    *_pp;               // opaque data struct (defined in vboxweb.cpp)
        bool                        _fDestructing;

        ManagedObjectRef            *_pISession;

        time_t                      _tLastObjectLookup;

        // hide the copy constructor because we're not copyable
        WebServiceSession(const WebServiceSession &copyFrom);

    public:
        WebServiceSession();

        ~WebServiceSession();

        int authenticate(const char *pcszUsername,
                         const char *pcszPassword);

        ManagedObjectRef* findRefFromPtr(const IUnknown *pObject);

        uint64_t getID() const
        {
            return _uSessionID;
        }

        WSDLT_ID getSessionObject() const;

        void touch();

        time_t getLastObjectLookup() const
        {
            return _tLastObjectLookup;
        }

        static WebServiceSession* findSessionFromRef(const WSDLT_ID &id);

        void DumpRefs();
};

/**
 *  ManagedObjectRef is used to map COM pointers to object IDs
 *  within a session. Such object IDs are 64-bit integers.
 *
 *  When a webservice method call is invoked on an object, it
 *  has an opaque string called a "managed object reference". Such
 *  a string consists of a session ID combined with an object ID.
 *
 */
class ManagedObjectRef
{
    protected:
        // owning session:
        WebServiceSession           &_session;

        void                        *_pvObj;                // COM object
        com::Guid                   _guidInterface;         // and the interface for which it was created

        const char                  *_pcszInterface;

        // keys:
        uint64_t                    _id;
        uintptr_t                   _ulp;

        // long ID as string
        WSDLT_ID                    _strID;

    public:
        ManagedObjectRef(WebServiceSession &session,
                         const char *pcszInterface,
                         IUnknown *pObject,
                         const com::Guid &guidInterface);
        ~ManagedObjectRef();

        uint64_t getID()
        {
            return _id;
        }

        /**
         * Returns the contained COM pointer and the UUID of the COM interface
         * which it supports.
         * @param
         * @return
         */
        const com::Guid& getPtr(void** pp)
        {
            *pp = _pvObj;
            return _guidInterface;
        }

        WSDLT_ID toWSDL() const;
        const char* getInterfaceName() const
        {
            return _pcszInterface;
        }

        static int findRefFromId(const WSDLT_ID &id,
                                 ManagedObjectRef **pRef,
                                 bool fNullAllowed);

        static ManagedObjectRef* findFromPtr(ComPtr<IUnknown> pcu);
        static ManagedObjectRef* create(const WSDLT_ID &idParent,
                                        ComPtr<IUnknown> pcu);

};

/**
 * Template function that resolves a managed object reference to a COM pointer
 * of the template class T. Gets called only from tons of generated code in
 * methodmaps.cpp.
 *
 * This is a template function so that we can support ComPtr's for arbitrary
 * interfaces and automatically verify that the managed object reference on
 * the internal stack actually is of the expected interface.
 *
 * @param soap
 * @param id in: integer managed object reference, as passed in by web service client
 * @param pComPtr out: reference to COM pointer object that receives the com pointer,
 *                if SOAP_OK is returned
 * @param fNullAllowed in: if true, then this func returns a NULL COM pointer if an
 *                empty MOR is passed in (i.e. NULL pointers are allowed). If false,
 *                then this fails; this will be false when called for the "this"
 *                argument of method calls, which really shouldn't be NULL.
 * @return error code or SOAP_OK if no error
 */
template <class T>
int findComPtrFromId(struct soap *soap,
                     const WSDLT_ID &id,
                     ComPtr<T> &pComPtr,
                     bool fNullAllowed)
{
    // findRefFromId requires thelock
    util::AutoWriteLock lock(g_pSessionsLockHandle COMMA_LOCKVAL_SRC_POS);

    int rc;
    ManagedObjectRef *pRef;
    if ((rc = ManagedObjectRef::findRefFromId(id, &pRef, fNullAllowed)))
        RaiseSoapInvalidObjectFault(soap, id);
    else
    {
        if (fNullAllowed && pRef == NULL)
        {
            WEBDEBUG(("   %s(): returning NULL object as permitted\n", __FUNCTION__));
            pComPtr.setNull();
            return 0;
        }

        const com::Guid &guidCaller = COM_IIDOF(T);

        // pRef->getPtr returns a void*
        void *p;
        const com::Guid &guidInterface = pRef->getPtr(&p);

        if (guidInterface == guidCaller)
        {
            // same interface: then no QueryInterface needed
            WEBDEBUG(("   %s(): returning pointer 0x%lX for original interface %s\n", __FUNCTION__, p, pRef->getInterfaceName()));
            pComPtr = (T*)p;            // calls AddRef() once
            return 0;
        }

        // QueryInterface tests whether p actually supports the templated T interface
        T *pT;
        ((IUnknown*)p)->QueryInterface(guidCaller, (void**)&pT);      // this adds a reference count
        if (pT)
        {
            // assign to caller's ComPtr<T>; use asOutParam() to avoid adding another reference
            WEBDEBUG(("   %s(): returning pointer 0x%lX for queried interface %RTuuid\n", __FUNCTION__, p, guidCaller.raw()));
            *(pComPtr.asOutParam()) = pT;
            return 0;
        }

        WEBDEBUG(("    Interface not supported for object reference %s, which is of class %s\n", id.c_str(), pRef->getInterfaceName()));
        rc = VERR_WEB_UNSUPPORTED_INTERFACE;
        RaiseSoapInvalidObjectFault(soap, id);      // @todo better message
    }

    return rc;
}

/**
 * Creates a new managed object for the given COM pointer. If a reference already exists
 * for the given pointer, then that reference's ID is returned instead.
 *
 * @param idParent managed object reference of calling object; used to extract session ID
 * @param pc COM object for which to create a reference
 * @return existing or new managed object reference
 */
template <class T>
WSDLT_ID createOrFindRefFromComPtr(const WSDLT_ID &idParent,
                                   const char *pcszInterface,
                                   ComPtr<T> &pc)
{
    // NULL comptr should return NULL MOR
    if (pc.isNull())
    {
        WEBDEBUG(("   createOrFindRefFromComPtr(): returning empty MOR for NULL COM pointer\n"));
        return "";
    }

    util::AutoWriteLock lock(g_pSessionsLockHandle COMMA_LOCKVAL_SRC_POS);
    WebServiceSession *pSession;
    if ((pSession = WebServiceSession::findSessionFromRef(idParent)))
    {
        ManagedObjectRef *pRef;
        ComPtr<IUnknown> pUnknown = pc;       // this calls QueryInterface(IUnknown)
        if (    ((pRef = pSession->findRefFromPtr(pUnknown)))
             || ((pRef = new ManagedObjectRef(*pSession,
                                              pcszInterface,
                                              pUnknown,              // IUnknown *pObject
                                              COM_IIDOF(T))))
           )
            return pRef->toWSDL();
    }

    // session has expired, return an empty MOR instead of allocating a
    // new reference which couldn't be used anyway.
    return "";
}
