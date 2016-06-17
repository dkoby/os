/*
 *     Yet another operating system for microcontrollers.
 *     Dynamic memory manipulation functions.
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
 * Allocate/free implementation with joining neighbour free blocks.
 *
 * NOTE Mutex is used to deny semultaneous access from different tasks.
 *      Don't run os_malloc() and os_mfree() from interrupts.
 *
 * NOTE
 *      Not tested well - be care.
 */
#include "os_private.h"
#include "os_mem.h"
#include "os_bitobj.h"

#ifdef OS_CONFIG_DYNMEM_2

struct block_t {
#define BLOCK_MAGIC    0xEFBE
    int magic;
#define BLOCK_STATE_FREE    0
#define BLOCK_STATE_BUSY    1
    int state;
    BASE_TYPE size;       /* overall size of block */
    struct block_t *next; /* pointer to next block */
};

#define ELEMENT_PAYLOAD_LEN(se) (se->len - sizeof(struct block_t))

struct os_dmem_stat_t os_dmstat;
static struct block_t *free_last;
#define MUTEX_MASK (1 << 0)
BASE_TYPE mutex;

/*
 *
 */
void *os_malloc(BASE_TYPE size)
{
    struct block_t *block;
    struct block_t *next;
    struct block_t *free;
    BASE_TYPE minsize;
    BASE_TYPE asize;
    void *ret;

    /* NOTE align size */
    size += (sizeof(BASE_TYPE) - (size % sizeof(BASE_TYPE)));

    ret  = NULL;
    free = NULL;

    os_mutex_lock(&mutex, MUTEX_MASK, OS_FLAG_NONE, OS_WAIT_FOREVER);

    /* size to allocate */
    asize = sizeof(struct block_t) + size;

    /* no one block was allocated yet */
    if (os_dmstat.blocks == 0)
    {
        free = (struct block_t*)&dmem;
        free->magic = BLOCK_MAGIC;
        free->size  = OS_CONFIG_DYNMEM_SIZE;
        free->next  = NULL;
        os_dmstat.blocks++;
        os_dmstat.free_blocks++;
    } else {
        if (free_last && free_last->state == BLOCK_STATE_FREE)
        {
            free = free_last;
        } else {
            /* lookup for free block with minimal space enough of required */
            minsize = OS_CONFIG_DYNMEM_SIZE;
            for (block = (struct block_t*)&dmem; block != NULL; block = block->next)
            {
#ifdef OS_CONFIG_TRAP_DYNMEM
                if (block->magic != BLOCK_MAGIC ||
                        (block->next != NULL && block->next->magic != BLOCK_MAGIC))
                {
                    os_trapinfo.err = OS_TRAP_ERR_DYNMEM_MAGIC;
                    os_trap();
                }
#endif
                next = block->next;
                /* join current and next blocks if they are both free */
                if (block->state == BLOCK_STATE_FREE &&
                        ((next != NULL) && (next->state == BLOCK_STATE_FREE)))
                {
                    next->magic = 0xADED; /* for debug purposes */
                    block->size += next->size;
                    block->next = next->next;

                    os_dmstat.blocks--;
                    os_dmstat.free_blocks--;
                }

                if (block->state == BLOCK_STATE_FREE &&
                        (block->size >= asize) && (block->size < minsize))
                {
                    minsize = block->size;
                    free = block;
                }
            }
        }
    }

#ifdef OS_CONFIG_TRAP_DYNMEM
    if (!free)
    {
        os_trapinfo.err = OS_TRAP_ERR_DYNMEM_EXHAUST;
        os_trap();
    }
#endif

    if (free)
    {
        /* if block contain enough space for another block then split it */
        if (free->size > (asize + sizeof(struct block_t)))
        {
            block = (struct block_t*)((BASE_TYPE)free + asize);
            block->magic = BLOCK_MAGIC;
            block->state = BLOCK_STATE_FREE;
            block->size  = free->size - asize;
            block->next  = free->next;

            free_last = block;

            free->size  = asize;
            free->next  = block;

            os_dmstat.blocks++;
            os_dmstat.free_blocks++;
        }
        free->state = BLOCK_STATE_BUSY;
        os_dmstat.memused += free->size;

        os_dmstat.free_blocks--;
        os_dmstat.busy_blocks++;
        ret = (void*)((BASE_TYPE)free + sizeof(struct block_t));
    }

    os_mutex_unlock(&mutex, MUTEX_MASK);
    return ret;
}

/*
 *
 */
void os_mfree(void *p)
{
    /* TODO */
    struct block_t *block;

    os_mutex_lock(&mutex, MUTEX_MASK, OS_FLAG_NONE, OS_WAIT_FOREVER);

    block = (struct block_t*)((BASE_TYPE)p - sizeof(struct block_t));
#ifdef OS_CONFIG_TRAP_DYNMEM
    if (block->magic != BLOCK_MAGIC)
    {
        os_trapinfo.err = OS_TRAP_ERR_DYNMEM_MAGIC;
        os_trap();
    }
    if (block->state != BLOCK_STATE_BUSY)
    {
        os_trapinfo.err = OS_TRAP_ERR_DYNMEM_DOUBLEFREE;
        os_trap();
    }
#endif
    block->state = BLOCK_STATE_FREE;
    os_dmstat.memused -= block->size;

    os_mutex_unlock(&mutex, MUTEX_MASK);
}

#endif /* OS_CONFIG_DYNMEM_2 */

