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

#include <arch/Types.h>
#include <arch/Startup.h>
#include <Compiler.h>

#define MAX_EXIT_FUNCS      32

typedef void (*constr_func)();

struct GlobalObj {
    void (*f)(void*);
    void *p;
    void *d;
};

/**
 * Will be called by crt0.S
 */
EXTERN_C void _init(int argc, char **argv);
/**
 * Will be called by gcc at the beginning for every global object to register the
 * destructor of the object
 */
EXTERN_C int __cxa_atexit(void (*f)(void *), void *p, void *d);
/**
 * We'll call this function in exit() to call all destructors registered by *atexit()
 */
EXTERN_C void __cxa_finalize(void *d);
/**
 * Is added by libgcc_eh and has to be called at startup
 */
EXTERN_C void __register_frame(const void *begin);

static size_t exitFuncCount = 0;
static GlobalObj exitFuncs[MAX_EXIT_FUNCS];
extern constr_func CTORS_BEGIN;
extern constr_func CTORS_END;
extern constr_func CTORS_REVERSE_BEGIN;
extern constr_func CTORS_REVERSE_END;
extern void *EH_FRAME_BEGIN;
void *__dso_handle;

void _init(int argc, char **argv) {
    // set program name for logging
    _startup_info.progname = argc == 0 ? "root" : argv[0];

    // init exception handling
    __register_frame(&EH_FRAME_BEGIN);

    // call reverse constructors (for clang)
    // these constructor calls seem to be only necessary on x86_32 and, for some strange reason,
    // on x86_64 when using a 64-bit host or so.
    // note also that clang seems to be unable to generate the correct code when "!=" is used
    // instead of "<". because in that case it simply ommits the check before entering the loop.
    for(constr_func *func = &CTORS_REVERSE_BEGIN; func < &CTORS_REVERSE_END; ++func)
        (*func)();
    // call constructors
    for(constr_func *func = &CTORS_END; func > &CTORS_BEGIN; )
        (*--func)();
}

int __cxa_atexit(void (*f)(void *), void *p, void *d) {
    if(exitFuncCount >= MAX_EXIT_FUNCS)
        return -1;

    exitFuncs[exitFuncCount].f = f;
    exitFuncs[exitFuncCount].p = p;
    exitFuncs[exitFuncCount].d = d;
    exitFuncCount++;
    return 0;
}

void __cxa_finalize(void *) {
    for(ssize_t i = exitFuncCount - 1; i >= 0; i--)
        exitFuncs[i].f(exitFuncs[i].p);
}
