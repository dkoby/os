/*
 *     Yet another operating system for microcontrollers.
 *     Private definiotions of OS.
 *
 * Copyright (c) 2013, Dmitry Kobylin
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#ifndef OS_PRIVATE_H
#define OS_PRIVATE_H

#include <types.h>
#include "os_config.h"
#include "os_flags.h"

/*
 * For debug purposes some variables should not be defined as static.
 * If debug is used undef DEBUG_STATIC definition.
 */
#define DEBUG_STATIC

#ifdef DEBUG_STATIC
    #define STATIC
#else
    #define STATIC static
#endif

// XXX obsolete macro, dprint function is used instead
//#define DEBUG_OS_PRINT

#ifdef DEBUG_OS_PRINT
    #include <debug.h>

    #define DEBUG_OS_STR(s) debug_str(s)
    #define DEBUG_OS_HEX(n, l) debug_hex(n, l)
    #define DEBUG_OS_HEX_WRAP(s, n, l) debug_hex_wrap(s, n, l)
#else
    #define DEBUG_OS_STR(s)
    #define DEBUG_OS_HEX(n, l)
    #define DEBUG_OS_HEX_WRAP(s, n, l)
#endif

#if PORT_ARMV7M
    #include "port/ARMv7-M/port.h"

    #define OS_CONTEXT_SIZE        (32 + 32) /* XXX should sizeof(os_task_context_t) be used instead ? */
    #define OS_STACK_MINSIZE       (OS_CONTEXT_SIZE + 32)
    #define OS_STACK_DIR_DECREASE
#else
    #error "unknown OS port"
#endif

#define OS_USE_LOCK (\
                        (defined OS_CONFIG_USE_WAIT)       || \
                        (defined OS_CONFIG_USE_TASK_SLICE) || \
                        (defined OS_CONFIG_USE_MUTEX)      || \
                        (defined OS_CONFIG_USE_EVENT)      || \
                        (defined OS_CONFIG_USE_QUEUE)      || \
                        (defined OS_CONFIG_USE_RMUTEX)        \
                       )
#define OS_USE_TIMEOUT (\
                        (defined OS_CONFIG_USE_WAIT)  || \
                        (defined OS_CONFIG_USE_MUTEX) || \
                        (defined OS_CONFIG_USE_EVENT) || \
                        (defined OS_CONFIG_USE_QUEUE) || \
                        (defined OS_CONFIG_USE_RMUTEX)   \
                       )

#if OS_USE_LOCK
struct os_task_lock_t {
    void *pobj;                /* pointer to object which cause lock of task */
    BASE_TYPE mask;            /* mask of mutex or event if lock type is mutex or event */

    /* locked task state mask */
    #define OS_TASK_LOCKED                   0x01
    /* task's states */
    #define OS_TASK_STATE_RUN                 0x00
    #define OS_TASK_STATE_SUSPEND             0x10
    #define OS_TASK_STATE_LOCKED_WAIT        (0x20 | OS_TASK_LOCKED)
    #define OS_TASK_STATE_LOCKED_MUTEX       (0x30 | OS_TASK_LOCKED)
    #define OS_TASK_STATE_LOCKED_EVENT       (0x40 | OS_TASK_LOCKED)
    #define OS_TASK_STATE_LOCKED_QUEUE_FULL  (0x50 | OS_TASK_LOCKED)
    #define OS_TASK_STATE_LOCKED_QUEUE_EMPTY (0x60 | OS_TASK_LOCKED)
    #define OS_TASK_STATE_LOCKED_MULTI       (0x70 | OS_TASK_LOCKED)
    #define OS_TASK_STATE_LOCKED_RMUTEX      (0x80 | OS_TASK_LOCKED)
    BASE_TYPE state;           /* type of lock or state of task */
};
#endif

/*
 * Task's control block
 */
struct os_taskcb_t {
    /*
     * Top/bottom of stack (depends on port's stack direction). Should be first
     * member of the structure.
     */
    uint8 *tos;

#if OS_USE_LOCK
    struct os_task_lock_t lock;

#if OS_USE_TIMEOUT
    BASE_TYPE timeout;
#endif

    volatile struct os_taskcb_t *prev;
    volatile struct os_taskcb_t *next;
#ifdef OS_CONFIG_USE_TASK_SLICE
    BASE_TYPE tslice;    /* time slice of task in units of OS_CONFIG_TICK_PERIOD */
    #ifdef OS_CONFIG_USE_VARIABLE_TASK_SLICE
    BASE_TYPE dslice;    /* slice to reload with */
    #endif
#endif /* OS_CONFIG_USE_TASK_SLICE */

#ifdef OS_CONFIG_USE_PRIORITY
    BASE_TYPE priority;
#endif

#endif /* OS_USE_LOCK */

#ifdef OS_CONFIG_TASK_NAME_SIZE
    char name[OS_CONFIG_TASK_NAME_SIZE]; /* task's symbolic name, used for debuging */
#endif

#ifdef OS_CONFIG_USE_TRACE
    uint8 *stack;     /* Pointer to start of stack, should be second member. */
    BASE_TYPE ssize;  /* Size of stack, should be third member. */

    BASE_TYPE schedhit; /* count of scheduler hits of this task */
    #ifdef OS_CONFIG_TICK_PERIOD
    BASE_TYPE ticks;     /* time of task in running state (system ticks) */
    #endif
    uint8 *tos_max;  /* maximum/minimum level of stack (depends on port's stack direction) */
    void (*process)();
#endif
    void *context;
};

struct os_trap_info_t {
    uint8 *tos;            /* last TOS, should be first member */
    
#define OS_TRAP_NOERR                    0x00
#define OS_TRAP_ERR_STACKMIN             0x01
#define OS_TRAP_ERR_TASKCOUNT            0x02
#define OS_TRAP_ERR_BITOBJIDX            0x03
#define OS_TRAP_ERR_SCHEDQ_UNKNOWN_LOCK  0x10
#define OS_TRAP_ERR_SCHEDQ_EMPTY_QUEUE   0x11
#define OS_TRAP_ERR_DYNMEM_EXHAUST       0x20
#define OS_TRAP_ERR_DYNMEM_MAGIC         0x21
#define OS_TRAP_ERR_DYNMEM_DOUBLEFREE    0x22
#define OS_TRAP_ERR_QUEUE_MSIZE          0x30
#define OS_TRAP_ERR_MULTI_ECOUNT         0x40
    BASE_TYPE err;         /* last error code, should be second member */
};

#ifdef OS_CONFIG_USE_SCHEDLOCK
extern volatile struct os_taskcb_t *os_schedlock_taskcb; /* pointer to task that possessed scheduler  */
#endif
extern volatile struct os_taskcb_t *os_current_taskcb;          /* pointer to current task's control block */
extern struct os_trap_info_t os_trapinfo;              /* trap inforamtion */
extern BASE_TYPE os_taskidx;                           /* number of current initialized tasks */

#define OS_IDLE_TASKCB (&os_tasks[0])
extern struct os_taskcb_t os_tasks[OS_CONFIG_TASK_COUNT + 1]; /* control block of task, NOTE os_task[0] is control block of idle task */

#ifdef OS_CONFIG_USE_TASK_SLICE
    inline void os_respawn_tslice(volatile struct os_taskcb_t *task);
#endif
    
void port_task_switch();
#define os_task_switch() port_task_switch()

#define OS_DISABLE_IRQ() PORT_DISABLE_IRQ()
#define OS_ENABLE_IRQ()  PORT_ENABLE_IRQ()

#define OS_TIMEOUT_EXPIRED    BASE_TYPE_MAX

void os_trap();
void os_tick();

#endif

