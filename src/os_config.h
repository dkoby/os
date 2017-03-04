/*
 *     Yet another operating system for MCU.
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
 *
 * 
 */
#ifndef OS_CONFIG_H
#define OS_CONFIG_H


#if 0
/* obsolete, TODO make proper description */
    /* ------------------------------------------------- */
    /* --- Configuration example                      -- */
    /* ------------------------------------------------- */

    #define OS_CONFIG_TASK_COUNT              2      /* number of tasks possible on system */
    #define OS_CONFIG_TICK_PERIOD             10000  /* period of OS timer, us */
    #define OS_CONFIG_USE_TASK_SLICE                 /* perform preemption when time slice of task expired */
    #define OS_CONFIG_DEFAULT_TASK_SLICE      2      /* time that task will be run before it gives control to other tasks, ticks */
    #define OS_CONFIG_USE_VARIABLE_TASK_SLICE        /* enable set of time slice of current task dynamicly */

    #define OS_CONFIG_USE_PRIORITY                   /* use priorities for tasks */

    #define OS_CONFIG_USE_WAIT                       /* enable os_wait() functionality*/
    #define OS_CONFIG_USE_MUTEX                      /* enable os_mutex_lock(), os_mutex_unlock() functionality */
    #define OS_CONFIG_USE_EVENT                      /* enable os_event_raise(), os_event_wait() functionality */
    #define OS_CONFIG_USE_RMUTEX                     /* enable os_rmutex_lock(), os_rmutex_unlock() functionality */
    #define OS_CONFIG_USE_MULTI                      /* enable multiple events funcitons */

    #define OS_CONFIG_USE_QUEUE                      /* enable os_queue_ functionality */

    #define OS_CONFIG_USE_VPREEMPT                   /* enable voluntary preemption */

    #define OS_CONFIG_USE_SCHEDLOCK                  /* enable os_sched_lock() and os_sched_unlock() functions */

    #define OS_CONFIG_USE_DYNMEM                     /* enable dynamic memory functions */
    #define OS_CONFIG_DYNMEM_SIZE  (16 * 1024 * 1024)/* size of dynamic memory */
    #define OS_CONFIG_DYNMEM_2                       /* second implementation of dynamic memory */

    #define OS_CONFIG_TRAP_SCHEDQ                    /* trap on scheduler queue errors */
    #define OS_CONFIG_TRAP_BITOBJ                    /* trap on bitobjects errors (mutexes, events) */
    #define OS_CONFIG_TRAP_DYNMEM                    /* trap on dynamic memory errors */
    #define OS_CONFIG_TRAP_QUEUE                     /* trap on queue errors */
    #define OS_CONFIG_TRAP_MULTI                     /* trap on multiple events errors */

#endif

/* check if proper configuration file speicifed */
#ifndef OS_CONFIGURATION_HEADER
    #error "OS_CONFIGURATION_HEADER macro not specified"
#endif

#include OS_CONFIGURATION_HEADER

#ifndef OS_BOARD_CONFIG
    #error "no proper OS configuration header included, check that OS_BOARD_CONFIG macro defined in header"
#endif

/*
 * check CONFIG dependencies
 */
#if (defined OS_CONFIG_USE_QUEUE) && (!(defined OS_CONFIG_USE_MUTEX) || !(defined OS_CONFIG_USE_DYNMEM))
    #error "OS_CONFIG_USE_QUEUE depends on OS_CONFIG_USE_DYNMEM && OS_CONFIG_USE_MUTEX"
#endif

#if (defined OS_CONFIG_USE_RMUTEX) && !(defined OS_CONFIG_USE_DYNMEM)
    #error "OS_CONFIG_USE_RMUTEX depends on OS_CONFIG_USE_DYNMEM"
#endif

#if (defined OS_CONFIG_USE_MULTI) && !(defined OS_CONFIG_USE_DYNMEM)
    #error "OS_CONFIG_USE_MULTI depends on OS_CONFIG_USE_DYNMEM"
#endif

#if (defined OS_CONFIG_USE_DYNMEM) && !( \
     (defined OS_CONFIG_DYNMEM_1) ||    \
     (defined OS_CONFIG_DYNMEM_2) ||    \
     (defined OS_CONFIG_DYNMEM_3)       \
        )
    #error "OS_CONFIG_USE_DYNMEM defined, one of OS_CONFIG_DYNMEM_x should be defined also"
#endif

#ifdef OS_CONFIG_DYNMEM_1
    #define OS_CONFIG_DYNMEM_IMPLEMENT
#endif
#if (defined OS_CONFIG_DYNMEM_2) && (defined OS_CONFIG_DYNMEM_IMPLEMENT)
    #error "only one of OS_CONFIG_DYNMEM_x should be used"
    #define OS_CONFIG_DYNMEM_IMPLEMENT
#endif
#if (defined OS_CONFIG_DYNMEM_3) && (defined OS_CONFIG_DYNMEM_IMPLEMENT)
    #error "only one of OS_CONFIG_DYNMEM_x should be used"
    #define OS_CONFIG_DYNMEM_IMPLEMENT
#endif

#if (defined OS_CONFIG_DYNMEM_2) && !(defined OS_CONFIG_USE_MUTEX)
    #error "OS_CONFIG_DYNMEM_2 depends on OS_CONFIG_USE_MUTEX"
#endif

#endif /* OS_CONFIG_H */

