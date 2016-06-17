/*
 *     Yet another operating system for microcontrollers.
 *     This file provide functions to implement support of ARMv7-M architecture
 *     (currently only Cortex-M3 tested).
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
#ifndef OS_CORTEXM3_PORT_H
#define OS_CORTEXM3_PORT_H

#include <types.h>

/*
 * context of task
 * NOTE should be packed
 */
struct os_task_context_t {
    /* context storead by software */
    BASE_TYPE r4;
    BASE_TYPE r5;
    BASE_TYPE r6;
    BASE_TYPE r7;
    BASE_TYPE r8;
    BASE_TYPE r9;
    BASE_TYPE r10;
    BASE_TYPE r11;
    /* context stored by ARMv7-M core */
    BASE_TYPE r0;
    BASE_TYPE r1;
    BASE_TYPE r2;
    BASE_TYPE r3;
    BASE_TYPE r12;
    BASE_TYPE lr;
    BASE_TYPE pc;
    BASE_TYPE xpsr;
};

extern const struct os_task_context_t icontext;

void PendSV_Handler(void);
void SVCall_Handler(void);
void SysTick_Handler(void);
void port_init();
void port_start();
void port_task_switch();
BASE_TYPE port_in_irq();

#define PORT_DISABLE_IRQ() \
        asm volatile("cpsid i\n")
#define PORT_ENABLE_IRQ()  \
        asm volatile(      \
            "cpsie i\n"    \
            "isb    \n"    \
        )
#define PORT_DATA_BARIER() \
        asm volatile(      \
                "dmb\n"    \
                "isb\n"    \
                )
/* XXX no need isb in DATA_BARIER */
 
#endif

