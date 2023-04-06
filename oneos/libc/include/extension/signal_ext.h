/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        signal_ext.h
 *
 * @brief       Supplement to the standard C library file.
 *
 * @revision
 * Date         Author          Notes
 * 2020-04-17   OneOS team      First Version
 ***********************************************************************************************************************
 */
#ifndef __SIGNAL_EXT_H__
#define __SIGNAL_EXT_H__

#include <sys/types.h>
#include <os_stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Signal Generation and Delivery, P1003.1b-1993, p. 63
 * NOTE: P1003.1c/D10, p. 34 adds sigev_notify_function and
 * sigev_notify_attributes to the sigevent structure.
 */
union sigval 
{
    int    sival_int;                           /* Integer signal value. */
    void  *sival_ptr;                           /* Pointer signal value. */
};

struct sigevent
{
    int            sigev_notify;                /* Notification type. */
    int            sigev_signo;                 /* Signal number. */
    union sigval   sigev_value;                 /* Signal value. */
    void         (*sigev_notify_function)( union sigval ); /* Notification function. */
    void          *sigev_notify_attributes;     /* Notification Attributes, really pthread_attr_t. */
};

struct siginfo
{
    int          si_signo;
    int          si_code;
    union sigval si_value;
};
typedef struct siginfo siginfo_t;

/* for si_code */
#define SI_USER         0x01                    /* Signal sent by kill(). */
#define SI_QUEUE        0x02                    /* Signal sent by sigqueue(). */
#define SI_TIMER        0x03                    /* Signal generated by expiration of a timer set by timer_settime(). */
#define SI_ASYNCIO      0x04                    /* Signal generated by completion of an asynchronous I/O request. */
#define SI_MESGQ        0x05                    /* Signal generated by arrival of a message on an empty message queue. */

#ifndef SIGEV_SIGNAL
#define SIGEV_SIGNAL    0                       /* notify via signal */
#endif

#ifndef SIGEV_NONE
#define SIGEV_NONE      1                       /* other notification: meaningless */
#endif

#ifndef SIGEV_THREAD
#define SIGEV_THREAD    2                       /* deliver via thread creation */
#endif

#if defined(__GNUC__)                           /* GNUC */

#if defined(OS_USING_NEWLIB_ADAPTER)            /* Newlib */
#include <signal.h>

#elif defined(OS_USING_MINILIB_ADAPTER)         /* Minilib */
/* TODO: Need Add */

#endif /* defined(OS_USING_NEWLIB_ADAPTER) */

#elif defined(__CC_ARM) || defined(__CLANG_ARM) /* Armcc or armclang */
#include <signal.h>

typedef unsigned long   sigset_t;

#define SIGHUP          1

#ifndef SIGINT
#define SIGINT          2
#endif

#define SIGQUIT         3

#ifndef SIGILL
#define SIGILL          4 
#endif

#define SIGTRAP         5

#ifndef SIGABRT
#define SIGABRT         6
#endif

#define SIGEMT          7

#ifndef SIGFPE
#define SIGFPE          8
#endif

#define SIGKILL         9
#define SIGBUS          10

#ifndef SIGSEGV
#define SIGSEGV         11
#endif

#define SIGSYS          12
#define SIGPIPE         13
#define SIGALRM         14

#ifndef SIGTERM
#define SIGTERM         15
#endif

#define SIGURG          16
#define SIGSTOP         17
#define SIGTSTP         18
#define SIGCONT         19
#define SIGCHLD         20
#define SIGTTIN         21
#define SIGTTOU         22
#define SIGPOLL         23
#define SIGWINCH        24

#ifndef SIGUSR1
#define SIGUSR1         25
#endif

#ifndef SIGUSR2
#define SIGUSR2         26
#endif

#define SIGRTMIN        27
#define SIGRTMAX        31
#define NSIG            32

#define SIG_SETMASK     0       /* Set mask with sigprocmask(). */
#define SIG_BLOCK       1       /* Set of signals to block. */
#define SIG_UNBLOCK     2       /* Set of signals to, well, unblock. */

typedef void (*_sig_func_ptr)(int);

struct sigaction 
{
    _sig_func_ptr sa_handler;
    sigset_t      sa_mask;
    int           sa_flags;
};

#define sigaddset(what,sig)     (*(what) |= (1 << (sig)), 0)
#define sigdelset(what,sig)     (*(what) &= ~(1 << (sig)), 0)
#define sigemptyset(what)       (*(what) = 0, 0)
#define sigfillset(what)        (*(what) = ~(0), 0)
#define sigismember(what,sig)   (((*(what)) & (1 << (sig))) != 0)

extern int sigprocmask(int how, const sigset_t *set, sigset_t *oset);
extern int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

#endif /* defined(__GNUC__)  */

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_EXT_H__ */

