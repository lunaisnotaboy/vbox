/** @file
 *
 * Automatic locks, implementation
 */

/*
 * Copyright (C) 2006-2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ____H_AUTOLOCK
#define ____H_AUTOLOCK

#include <iprt/types.h>

// macros for automatic lock validation; these will amount to nothing
// unless lock validation is enabled for the runtime
#if defined(RT_LOCK_STRICT)
# define VBOX_WITH_MAIN_LOCK_VALIDATION
# define COMMA_LOCKVAL_SRC_POS , RT_SRC_POS
# define LOCKVAL_SRC_POS_DECL RT_SRC_POS_DECL
# define COMMA_LOCKVAL_SRC_POS_DECL , RT_SRC_POS_DECL
# define LOCKVAL_SRC_POS_ARGS RT_SRC_POS_ARGS
# define COMMA_LOCKVAL_SRC_POS_ARGS , RT_SRC_POS_ARGS
#else
# define COMMA_LOCKVAL_SRC_POS
# define LOCKVAL_SRC_POS_DECL
# define COMMA_LOCKVAL_SRC_POS_DECL
# define LOCKVAL_SRC_POS_ARGS
# define COMMA_LOCKVAL_SRC_POS_ARGS
#endif

namespace util
{

////////////////////////////////////////////////////////////////////////////////
//
// Order classes for lock validation
//
////////////////////////////////////////////////////////////////////////////////

/**
 * IPRT now has a sophisticated system of run-time locking classes to validate
 * locking order. Since the Main code is handled by simpler minds, we want
 * compile-time constants for simplicity, and we'll look up the run-time classes
 * in AutoLock.cpp transparently. These are passed to the constructors of the
 * LockHandle classes.
 */
enum VBoxLockingClass
{
    LOCKCLASS_NONE = 0,
    LOCKCLASS_VIRTUALBOXOBJECT = 1,         // highest order: VirtualBox object lock
    LOCKCLASS_HOSTOBJECT = 2,               // Host object lock
    LOCKCLASS_LISTOFMACHINES = 3,           // list of machines in VirtualBox object
    LOCKCLASS_MACHINEOBJECT = 4,            // Machine object lock
    LOCKCLASS_SNAPSHOTOBJECT = 5,           // snapshot object locks
                                            // (the snapshots tree, including the child pointers in Snapshot,
                                            // is protected by the normal Machine object lock)
    LOCKCLASS_LISTOFMEDIA = 6,              // list of media (hard disks, DVDs, floppies) in VirtualBox object
    LOCKCLASS_LISTOFOTHEROBJECTS = 7,       // any other list of objects
    LOCKCLASS_OTHEROBJECT = 8,              // any regular object member variable lock
    LOCKCLASS_USBLIST = 9,                 // temporary hack to avoid having to clean up the USB filters
                                            // too much @todo r=dj get rid of this!
    LOCKCLASS_PROGRESSLIST = 10,            // list of progress objects in VirtualBox; no other object lock
                                            // may be held after this!
    LOCKCLASS_OBJECTSTATE = 11              // object state lock (handled by AutoCaller classes)
};

void InitAutoLockSystem();

////////////////////////////////////////////////////////////////////////////////
//
// LockHandle and friends
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for semaphore handles (RWLockHandle and WriteLockHandle).
 * Don't use this directly, but this implements lock validation for them.
 */
class LockHandle
{
public:
    LockHandle()
    {}

    virtual ~LockHandle()
    {}

    /**
     * Returns @c true if the current thread holds a write lock on this
     * read/write semaphore. Intended for debugging only.
     */
    virtual bool isWriteLockOnCurrentThread() const = 0;

    /**
     * Returns the current write lock level of this semaphore. The lock level
     * determines the number of nested #lock() calls on the given semaphore
     * handle.
     *
     * Note that this call is valid only when the current thread owns a write
     * lock on the given semaphore handle and will assert otherwise.
     */
    virtual uint32_t writeLockLevel() const = 0;

    virtual void lockWrite(LOCKVAL_SRC_POS_DECL) = 0;
    virtual void unlockWrite() = 0;
    virtual void lockRead(LOCKVAL_SRC_POS_DECL) = 0;
    virtual void unlockRead() = 0;

#ifdef VBOX_WITH_MAIN_LOCK_VALIDATION
    virtual const char* describe() const = 0;
#endif

private:
    // prohibit copy + assignment
    LockHandle(const LockHandle&);
    LockHandle& operator=(const LockHandle&);
};

/**
 * Full-featured read/write semaphore handle implementation.
 *
 * This is an auxiliary base class for classes that need full-featured
 * read/write locking as described in the AutoWriteLock class documentation.
 * Instances of classes inherited from this class can be passed as arguments to
 * the AutoWriteLock and AutoReadLock constructors.
 */
class RWLockHandle : public LockHandle
{
public:
    RWLockHandle(VBoxLockingClass lockClass);
    virtual ~RWLockHandle();

    virtual bool isWriteLockOnCurrentThread() const;

    virtual void lockWrite(LOCKVAL_SRC_POS_DECL);
    virtual void unlockWrite();
    virtual void lockRead(LOCKVAL_SRC_POS_DECL);
    virtual void unlockRead();

    virtual uint32_t writeLockLevel() const;

#ifdef VBOX_WITH_MAIN_LOCK_VALIDATION
    virtual const char* describe() const;
#endif

private:
    struct Data;
    Data *m;
};

/**
 * Write-only semaphore handle implementation.
 *
 * This is an auxiliary base class for classes that need write-only (exclusive)
 * locking and do not need read (shared) locking. This implementation uses a
 * cheap and fast critical section for both lockWrite() and lockRead() methods
 * which makes a lockRead() call fully equivalent to the lockWrite() call and
 * therefore makes it pointless to use instahces of this class with
 * AutoReadLock instances -- shared locking will not be possible anyway and
 * any call to lock() will block if there are lock owners on other threads.
 *
 * Use with care only when absolutely sure that shared locks are not necessary.
 */
class WriteLockHandle : public LockHandle
{
public:
    WriteLockHandle(VBoxLockingClass lockClass);
    virtual ~WriteLockHandle();
    virtual bool isWriteLockOnCurrentThread() const;

    virtual void lockWrite(LOCKVAL_SRC_POS_DECL);
    virtual void unlockWrite();
    virtual void lockRead(LOCKVAL_SRC_POS_DECL);
    virtual void unlockRead();
    virtual uint32_t writeLockLevel() const;

#ifdef VBOX_WITH_MAIN_LOCK_VALIDATION
    virtual const char* describe() const;
#endif

private:
    struct Data;
    Data *m;
};

////////////////////////////////////////////////////////////////////////////////
//
// Lockable
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Lockable interface.
 *
 * This is an abstract base for classes that need read/write locking. Unlike
 * RWLockHandle and other classes that makes the read/write semaphore a part of
 * class data, this class allows subclasses to decide which semaphore handle to
 * use.
 */
class Lockable
{
public:

    /**
     * Returns a pointer to a LockHandle used by AutoWriteLock/AutoReadLock
     * for locking. Subclasses are allowed to return @c NULL -- in this case,
     * the AutoWriteLock/AutoReadLock object constructed using an instance of
     * such subclass will simply turn into no-op.
     */
    virtual LockHandle *lockHandle() const = 0;

    /**
     * Equivalent to <tt>#lockHandle()->isWriteLockOnCurrentThread()</tt>.
     * Returns @c false if lockHandle() returns @c NULL.
     */
    bool isWriteLockOnCurrentThread()
    {
        LockHandle *h = lockHandle();
        return h ? h->isWriteLockOnCurrentThread() : false;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoLockBase
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for all autolocks.
 *
 * This cannot be used directly. Use AutoReadLock or AutoWriteLock or AutoMultiWriteLock2/3
 * which directly and indirectly derive from this.
 *
 * In the implementation, the instance data contains a list of lock handles.
 * The class provides some utility functions to help locking and unlocking
 * them.
 */

class AutoLockBase
{
protected:
    AutoLockBase(uint32_t cHandles
                 COMMA_LOCKVAL_SRC_POS_DECL);
    AutoLockBase(uint32_t cHandles,
                 LockHandle *pHandle
                 COMMA_LOCKVAL_SRC_POS_DECL);
    virtual ~AutoLockBase();

    struct Data;
    Data *m;

    virtual void callLockImpl(LockHandle &l) = 0;
    virtual void callUnlockImpl(LockHandle &l) = 0;

    void callLockOnAllHandles();
    void callUnlockOnAllHandles();

    void cleanup();

public:
    void acquire();
    void release();

private:
    // prohibit copy + assignment
    AutoLockBase(const AutoLockBase&);
    AutoLockBase& operator=(const AutoLockBase&);
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoReadLock
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Automatic read lock. Use this with a RWLockHandle to request a read/write
 * semaphore in read mode. You can also use this with a WriteLockHandle but
 * that makes little sense since they treat read mode like write mode.
 *
 * If constructed with a RWLockHandle or an instance of Lockable (which in
 * practice means any VirtualBoxBase derivative), it autoamtically requests
 * the lock in read mode and releases the read lock in the destructor.
 */
class AutoReadLock : public AutoLockBase
{
public:

    /**
     * Constructs a null instance that does not manage any read/write
     * semaphore.
     *
     * Note that all method calls on a null instance are no-ops. This allows to
     * have the code where lock protection can be selected (or omitted) at
     * runtime.
     */
    AutoReadLock(LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       NULL
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(LockHandle *aHandle
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       aHandle
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(LockHandle &aHandle
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       &aHandle
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(const Lockable &aLockable
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       aLockable.lockHandle()
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a read lock.
     */
    AutoReadLock(const Lockable *aLockable
                 COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(1,
                       aLockable ? aLockable->lockHandle() : NULL
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    virtual ~AutoReadLock();

    virtual void callLockImpl(LockHandle &l);
    virtual void callUnlockImpl(LockHandle &l);
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoWriteLockBase
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Base class for all auto write locks.
 *
 * This cannot be used directly. Use AutoWriteLock or AutoMultiWriteLock2/3
 * which derive from this.
 *
 * In addition to utility methods for subclasses, this implements the public
 * leave/enter methods, which are common to all
 * write locks.
 */
class AutoWriteLockBase : public AutoLockBase
{
protected:
    AutoWriteLockBase(uint32_t cHandles
                      COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(cHandles
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    AutoWriteLockBase(uint32_t cHandles,
                      LockHandle *pHandle
                      COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoLockBase(cHandles,
                       pHandle
                       COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    virtual ~AutoWriteLockBase()
    { }

    virtual void callLockImpl(LockHandle &l);
    virtual void callUnlockImpl(LockHandle &l);

public:
    void leave();
    void enter();
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoWriteLock
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Automatic write lock. Use this with a RWLockHandle to request a read/write
 * semaphore in write mode. There can only ever be one writer of a read/write
 * semaphore: while the lock is held in write mode, no other writer or reader
 * can request the semaphore and will block.
 *
 * If constructed with a RWLockHandle or an instance of Lockable (which in
 * practice means any VirtualBoxBase derivative), it autoamtically requests
 * the lock in write mode and releases the write lock in the destructor.
 *
 * When used with a WriteLockHandle, it requests the semaphore contained therein
 * exclusively.
 */
class AutoWriteLock : public AutoWriteLockBase
{
public:

    /**
     * Constructs a null instance that does not manage any read/write
     * semaphore.
     *
     * Note that all method calls on a null instance are no-ops. This allows to
     * have the code where lock protection can be selected (or omitted) at
     * runtime.
     */
    AutoWriteLock(LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            NULL
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    { }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(LockHandle *aHandle
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            aHandle
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(LockHandle &aHandle
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            &aHandle
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(const Lockable &aLockable
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            aLockable.lockHandle()
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Constructs a new instance that will start managing the given read/write
     * semaphore by requesting a write lock.
     */
    AutoWriteLock(const Lockable *aLockable
                  COMMA_LOCKVAL_SRC_POS_DECL)
        : AutoWriteLockBase(1,
                            aLockable ? aLockable->lockHandle() : NULL
                            COMMA_LOCKVAL_SRC_POS_ARGS)
    {
        acquire();
    }

    /**
     * Release all write locks acquired by this instance through the #lock()
     * call and destroys the instance.
     *
     * Note that if there there are nested #lock() calls without the
     * corresponding number of #unlock() calls when the destructor is called, it
     * will assert. This is because having an unbalanced number of nested locks
     * is a program logic error which must be fixed.
     */
    virtual ~AutoWriteLock()
    {
        cleanup();
    }

    void attach(LockHandle *aHandle);

    /** @see attach (LockHandle *) */
    void attach(LockHandle &aHandle)
    {
        attach(&aHandle);
    }

    /** @see attach (LockHandle *) */
    void attach(const Lockable &aLockable)
    {
        attach(aLockable.lockHandle());
    }

    /** @see attach (LockHandle *) */
    void attach(const Lockable *aLockable)
    {
        attach(aLockable ? aLockable->lockHandle() : NULL);
    }

    bool isWriteLockOnCurrentThread() const;
    uint32_t writeLockLevel() const;
};

////////////////////////////////////////////////////////////////////////////////
//
// AutoMultiWriteLock*
//
////////////////////////////////////////////////////////////////////////////////

/**
 * A multi-write-lock containing two other write locks.
 *
 */
class AutoMultiWriteLock2 : public AutoWriteLockBase
{
public:
    AutoMultiWriteLock2(Lockable *pl1,
                        Lockable *pl2
                        COMMA_LOCKVAL_SRC_POS_DECL);
    AutoMultiWriteLock2(LockHandle *pl1,
                        LockHandle *pl2
                        COMMA_LOCKVAL_SRC_POS_DECL);

    virtual ~AutoMultiWriteLock2()
    {
        cleanup();
    }
};

/**
 * A multi-write-lock containing three other write locks.
 *
 */
class AutoMultiWriteLock3 : public AutoWriteLockBase
{
public:
    AutoMultiWriteLock3(Lockable *pl1,
                        Lockable *pl2,
                        Lockable *pl3
                        COMMA_LOCKVAL_SRC_POS_DECL);
    AutoMultiWriteLock3(LockHandle *pl1,
                        LockHandle *pl2,
                        LockHandle *pl3
                        COMMA_LOCKVAL_SRC_POS_DECL);

    virtual ~AutoMultiWriteLock3()
    {
        cleanup();
    }
};

} /* namespace util */

#endif // ____H_AUTOLOCK

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
