/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/Pd.h>
#include <kobj/GlobalThread.h>
#include <kobj/Pt.h>
#include <kobj/UserSm.h>
#include <ipc/ClientSession.h>
#include <subsystem/ChildMemory.h>
#include <subsystem/ServiceRegistry.h>
#include <collection/SList.h>
#include <collection/SListTreap.h>
#include <region/PortManager.h>
#include <bits/BitField.h>
#include <String.h>

namespace nre {

class OStream;
class Child;
class ChildManager;

OStream &operator<<(OStream &os, const Child &c);

/**
 * Represents a running child task. This is used by the ChildManager to keep track of all its
 * children. You can't create instances of it but only get instances from the ChildManager.
 */
class Child : public SListTreapNode<size_t>, public RefCounted {
    friend class ChildManager;
    friend OStream & operator<<(OStream &os, const Child &c);

    /**
     * Holds the properties of an Sc that has been announced by a child
     */
    class SchedEntity : public SListItem {
    public:
        /**
         * Creates a new sched-entity
         *
         * @param id the thread id
         * @param name the name of the thread
         * @param cpu the cpu its running on
         * @param cap the Sc capability
         */
        explicit SchedEntity(void *ptr, const String &name, cpu_t cpu, capsel_t cap)
            : SListItem(), _ptr(ptr), _name(name), _cpu(cpu), _cap(cap) {
        }

        /**
         * @return the id of the thread (unique within one child)
         */
        void *ptr() const {
            return _ptr;
        }
        /**
         * @return the name of the thread
         */
        const String &name() const {
            return _name;
        }
        /**
         * @return the cpu its running on
         */
        cpu_t cpu() const {
            return _cpu;
        }
        /**
         * @return the Sc capability
         */
        capsel_t cap() const {
            return _cap;
        }

    private:
        void *_ptr;
        String _name;
        cpu_t _cpu;
        capsel_t _cap;
    };

    /**
     * Used to wait for the termination of GlobalThreads.
     */
    class JoinItem : public SListItem {
    public:
        /**
         * Constructor
         *
         * @param ptr the id of the thread
         * @param sm the sm-capability to up as soon as the thread died
         */
        explicit JoinItem(void *ptr, capsel_t sm) : SListItem(), _ptr(ptr), _sm(sm, true) {
        }

        /**
         * @return the id of the thread (unique within one child)
         */
        void *ptr() const {
            return _ptr;
        }
        /**
         * @return the semaphore
         */
        Sm &sm() {
            return _sm;
        }

    private:
        void *_ptr;
        Sm _sm;
    };

public:
    typedef size_t id_type;

    /**
     * @return the id of this child
     */
    id_type id() const {
        return _id;
    }
    /**
     * @return the cpu its main-thread is running on
     */
    cpu_t cpu() const {
        return _ec->cpu();
    }
    /**
     * @return the protection domain cap
     */
    capsel_t pd() const {
        return _pd->sel();
    }
    /**
     * @return its command line
     */
    const String &cmdline() const {
        return _cmdline;
    }

    /**
     * @return the entry-point (0 = main)
     */
    uintptr_t entry() const {
        return _entry;
    }
    /**
     * @return the stack address of the main thread
     */
    uintptr_t stack() const {
        return _stack;
    }
    /**
     * @return the UTCB address of the main thread
     */
    uintptr_t utcb() const {
        return _utcb;
    }
    /**
     * @return the address of the Hip
     */
    uintptr_t hip() const {
        return _hip;
    }

    /**
     * @return the virtual memory regions
     */
    ChildMemory &reglist() {
        return _regs;
    }
    const ChildMemory &reglist() const {
        return _regs;
    }

    /**
     * @return the allocated IO ports
     */
    PortManager &io() {
        return _io;
    }
    const PortManager &io() const {
        return _io;
    }

    /**
     * @return the allocated GSIs
     */
    BitField<Hip::MAX_GSIS> &gsis() {
        return _gsis;
    }
    const BitField<Hip::MAX_GSIS> &gsis() const {
        return _gsis;
    }

    /**
     * @return the announced Scs
     */
    SList<SchedEntity> &scs() {
        return _scs;
    }
    const SList<SchedEntity> &scs() const {
        return _scs;
    }

    /**
     * Opens a session at service <name>, if allowed.
     *
     * @param name the name of the service
     * @param args the session arguments
     * @param s if known, the service, nullptr otherwise#
     * @return the created handle
     * @throws Exception if not allowed
     */
    const ClientSession *open_session(const String &name, const String &args,
                                      const ServiceRegistry::Service *s);
    /**
     * Closes the session identified by given handle.
     *
     * @param handle the handle
     * @throws Exception if not found
     */
    void close_session(capsel_t handle);

private:
    explicit Child(ChildManager *cm, id_type id, const String &cmdline)
        : SListTreapNode<size_t>(id), RefCounted(), _cm(cm), _id(id), _cmdline(cmdline), _started(),
          _pd(), _ec(), _pts(), _ptcount(), _regs(), _io(PortManager::USED), _scs(), _gsis(),
          _sessions(), _joins(),  _gsi_caps(CapSelSpace::get().allocate(Hip::MAX_GSIS)),
          _gsi_next(), _entry(), _main(), _stack(), _utcb(), _hip(), _sm() {
    }
public:
    virtual ~Child();

private:
    void destroy() {
        for(size_t i = 0; i < _ptcount; ++i)
            delete _pts[i];
    }

    void alloc_thread(uintptr_t *stack_addr, uintptr_t *utcb_addr);
    capsel_t create_thread(capsel_t ec, const String &name, void *ptr, cpu_t cpu, Qpd &qpd);
    SchedEntity *get_thread_by_id(void *ptr);
    SchedEntity *get_thread_by_cap(capsel_t cap);
    void join_thread(void *ptr, capsel_t sm);
    void term_thread(void *ptr, uintptr_t stack, uintptr_t utcb);
    void remove_thread(capsel_t cap);
    void destroy_thread(SchedEntity *se);
    void destroy_sc(capsel_t cap);

    void release_gsis();
    void release_ports();
    void release_scs();
    void release_regs();
    void release_sessions();

    Child(const Child&);
    Child& operator=(const Child&);

    ChildManager *_cm;
    id_type _id;
    String _cmdline;
    bool _started;
    Pd *_pd;
    Reference<GlobalThread> _ec;
    Pt **_pts;
    size_t _ptcount;
    ChildMemory _regs;
    PortManager _io;
    SList<SchedEntity> _scs;
    BitField<Hip::MAX_GSIS> _gsis;
    SList<ClientSession> _sessions;
    SList<JoinItem> _joins;
    capsel_t _gsi_caps;
    capsel_t _gsi_next;
    uintptr_t _entry;
    uintptr_t _main;
    uintptr_t _stack;
    uintptr_t _utcb;
    uintptr_t _hip;
    UserSm _sm;
};

}
