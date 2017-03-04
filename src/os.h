/*
 *     Yet another operating system for microcontrollers.
 *     Public interface of OS. This file shoul be included in user application.
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
#ifndef OS_H
#define OS_H

#include <types.h>
#include "os_config.h"
#include "os_flags.h"

void os_init();
#ifdef OS_CONFIG_USE_PRIORITY
    struct os_taskcb_t* os_task_init(char *name, uint8 *stack, BASE_TYPE ssize, BASE_TYPE priority, void (*process)(), void *context);
    #define OS_TASK_INIT(name, stack, ssize, priority, process, context) os_task_init(name, stack, ssize, priority, process, context);
#else
    struct os_taskcb_t* os_task_init(char *name, uint8 *stack, BASE_TYPE ssize, void (*process)(), void *context);
    #define OS_TASK_INIT(name, stack, ssize, priority, process, context) os_task_init(name, stack, ssize, process, context);
#endif

void os_start();

#define OS_MS2TICK(ms) ((((ms) * 1000 / OS_CONFIG_TICK_PERIOD) > 1) ? ((ms) * 1000 / OS_CONFIG_TICK_PERIOD) : (ms > 0 ? 2 : OS_WAIT_FOREVER))
#define OS_US2TICK(us) ((((us)        / OS_CONFIG_TICK_PERIOD) > 1) ? ((us)        / OS_CONFIG_TICK_PERIOD) : (us > 0 ? 2 : OS_WAIT_FOREVER))

#ifdef OS_CONFIG_USE_WAIT
    void os_wait(BASE_TYPE ticks);

    #define os_wait_ms(ms) os_wait(OS_MS2TICK(ms))
    #define os_wait_us(us) os_wait(OS_US2TICK(us))
#endif

void os_yield();
#ifdef OS_CONFIG_USE_VPREEMPT
void os_vyield();
#endif

#if (defined OS_CONFIG_USE_MUTEX) || (defined OS_CONFIG_USE_EVENT)
    #include "os_bitobj.h"
#endif
#ifdef OS_CONFIG_USE_RMUTEX
    #include "os_rmutex.h"
#endif

#if (defined OS_CONFIG_USE_TASK_SLICE) && (defined OS_CONFIG_USE_VARIABLE_TASK_SLICE)
    void os_set_slice(BASE_TYPE ticks);
    #define os_set_slice_ms(ms) os_set_slice(OS_MS2TICK(ms))
    #define os_set_slice_us(ms) os_set_slice(OS_US2TICK(us))
#endif

#ifdef OS_CONFIG_USE_QUEUE
    #include "os_queue.h"
#endif

#ifdef OS_CONFIG_USE_DYNMEM
    #include "os_mem.h"
#endif

#ifdef OS_CONFIG_USE_MULTI
    #include "os_multi.h"
#endif

#ifdef OS_CONFIG_USE_TRACE
    #include "os_private.h"
#endif

#ifdef OS_CONFIG_USE_SCHEDLOCK
    void os_sched_lock();
    void os_sched_unlock();
#endif

void os_disable_irq();
void os_enable_irq();

void os_disable_irq_r();
void os_enable_irq_r();

void *os_task_get_context();

#endif

