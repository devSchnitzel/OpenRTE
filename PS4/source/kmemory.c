// This file is a part of OpenRTE
// Version: 1.0
// Author: TheoryWrong
// Thanks to: CTurt, Zecoxao, wskeu, BadChoicez, wildcard, Z80 and all people contribuing to ps4 scene
// Also thanks to: Dev_Shootz, MsKx, Marbella, JimmyModding, AZN, MarentDev (Modder/Tester)
// This file is under GNU licence

#undef _SYS_CDEFS_H_
#undef _SYS_TYPES_H_
#undef _SYS_PARAM_H_
#undef _SYS_MALLOC_H_

#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1
#define _KERNEL
#define _WANT_UCRED

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/fcntl.h>

#include <sys/proc.h>
#include <sys/ptrace.h>

#undef offsetof
#include <kernel.h>
#include <ps4/kernel.h>

#include "kmemory.h"

int read_mem(struct thread *td, struct proc *p, uint64_t base, char *buff, int size) {
    struct uio uio;
    struct iovec iov;
    iov.iov_base = (void*)buff;
    iov.iov_len = size;

    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_offset = base;
    uio.uio_resid = size;
    uio.uio_segflg = UIO_SYSSPACE;
    uio.uio_rw = UIO_READ;
    uio.uio_td = td;

    int ret = proc_rwmem(p, &uio);
    return ret;
}

int write_mem(struct thread *td, struct proc *p, uint64_t base, char *buff, int size) {
    struct uio uio;
    struct iovec iov;
    iov.iov_base = (void*)buff;
    iov.iov_len = size;

    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_offset = base;
    uio.uio_resid = size;
    uio.uio_segflg = UIO_SYSSPACE;
    uio.uio_rw = UIO_WRITE;
    uio.uio_td = td;

    int ret = proc_rwmem(p, &uio);
    return ret;
}

int sys_read_process_mem(struct thread *td, void *uap) {
    char *mem = (char *)uap;    // ** user memory
    struct malloc_type *mt = ps4KernelDlSym("M_TEMP");
    void *kmem = malloc(sizeof(struct ReadMemArgs), mt, M_ZERO | M_WAITOK);
    struct ReadMemArgs *args = (struct ReadMemArgs*)kmem;
    copyin(mem, kmem, sizeof(struct ReadMemArgs));

    struct proc *p = pfind(args->pid);
    if (p != NULL) {
        void *buff = malloc(args->len, mt, M_ZERO | M_WAITOK);
        int ret = read_mem(td, p, args->offset, buff, args->len);
        if (ret == 0) {
            copyout(buff, args->buff, args->len);
        }
        free(buff, mt);

        PROC_UNLOCK(p);
        ps4KernelThreadSetReturn(td, ret);
    }
    else {
        ps4KernelThreadSetReturn(td, -1);
    }

    free(kmem, mt);
    return 0;
}

int sys_write_process_mem(struct thread *td, void *uap) {
    char *mem = (char *)uap;    // ** user memory
    struct malloc_type *mt = ps4KernelDlSym("M_TEMP");
    void *kmem = malloc(sizeof(struct WriteMemArgs), mt, M_ZERO | M_WAITOK);
    struct WriteMemArgs *args = (struct WriteMemArgs*)kmem;
    copyin(mem, kmem, sizeof(struct WriteMemArgs));

    struct proc *p = pfind(args->pid);
    if (p != NULL) {
        void *buff = malloc(args->len, mt, M_ZERO | M_WAITOK);
        copyin(args->buff, buff, args->len);
        int ret = write_mem(td, p, args->offset, buff, args->len);
        free(buff, mt);

        PROC_UNLOCK(p);
        ps4KernelThreadSetReturn(td, ret);
    }
    else {
        ps4KernelThreadSetReturn(td, -1);
    }

    free(kmem, mt);
    return 0;
}

int sys_beep(struct thread *td, void *uap) {
    void* f_addr;
    ps4KernelSymbolLookUp("icc_indicator_set_buzzer", &f_addr);
    if (f_addr == NULL) {
        ps4KernelThreadSetReturn(td, 1);
        return 0;
    }
    ((generic_int)f_addr)(1);

    ps4KernelThreadSetReturn(td, 0);
    return 0;
}

int sys_redled(struct thread *td, void *uap) {
    void* f_addr1;
    void* f_addr2;

    ps4KernelSymbolLookUp("icc_indicator_set_dynamic_led_off", &f_addr1);
    ps4KernelSymbolLookUp("icc_indicator_set_thermalalert_led_on", &f_addr2);
    if (f_addr1 == NULL && f_addr2 == NULL) {
        ps4KernelThreadSetReturn(td, 1);
        return 0;
    }

    ((generic)f_addr1)();
    ((generic)f_addr2)();

    ps4KernelThreadSetReturn(td, 0);
    return 0;
}  

int sys_blueled(struct thread *td, void *uap) {
    void* f_addr1;

    ps4KernelSymbolLookUp("icc_indicator_set_dynamic_led_standby_boot", &f_addr1);
    if (f_addr1 == NULL) {
        ps4KernelThreadSetReturn(td, 1);
        return 0;
    }

    ((generic)f_addr1)();

    ps4KernelThreadSetReturn(td, 0);
    return 0;
}

int sys_orangeled(struct thread *td, void *uap) {
    void* f_addr1;

    ps4KernelSymbolLookUp("icc_indicator_set_dynamic_led_standby", &f_addr1);
    if (f_addr1 == NULL) {
        ps4KernelThreadSetReturn(td, 1);
        return 0;
    }

    ((generic)f_addr1)();

    ps4KernelThreadSetReturn(td, 0);
    return 0;
}

int sys_whiteled(struct thread *td, void *uap) {
    void* f_addr1;

    ps4KernelSymbolLookUp("icc_indicator_set_dynamic_led_boot", &f_addr1);
    if (f_addr1 == NULL) {
        ps4KernelThreadSetReturn(td, 1);
        return 0;
    }

    ((generic)f_addr1)();

    ps4KernelThreadSetReturn(td, 0);
    return 0;
}