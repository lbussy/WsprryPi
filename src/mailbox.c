/**
 * @file mailbox.c
 * @brief Implementation of mailbox-based communication for the Raspberry Pi.
 * 
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).  However the mailbox implementation is property of Broadcom.
 * 
 * Copyright (c) 2012, Broadcom Europe Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  - Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * References:
 * - https://github.com/raspberrypi/firmware/wiki/Mailboxes
 * - https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 * - https://bitbanged.com/posts/understanding-rpi/the-mailbox/
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "mailbox.h"

#define PAGE_SIZE (4 * 1024)

/**
 * @brief Maps physical memory into the process's address space.
 *
 * @param base Physical base address to map.
 * @param size Size of the memory region to map.
 * @return Pointer to the mapped memory, or exits on failure.
 */
void *mapmem(uint32_t base, uint32_t size)
{
    int mem_fd;
    unsigned offset = base % PAGE_SIZE;
    base -= offset;

    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        fprintf(stderr, "Error: Cannot open /dev/mem. Run as root or use sudo.\n");
        exit(EXIT_FAILURE);
    }

    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, base);
    close(mem_fd);

    if (mem == MAP_FAILED)
    {
        perror("Error: mmap failed");
        exit(EXIT_FAILURE);
    }

    return (uint8_t *)mem + offset;
}

/**
 * @brief Unmaps previously mapped memory.
 *
 * @param addr Pointer to the mapped memory.
 * @param size Size of the memory region to unmap.
 */
void unmapmem(void *addr, uint32_t size)
{
    if (munmap(addr, size) != 0)
    {
        perror("Error: munmap failed");
        exit(EXIT_FAILURE);
    }
}

/*
 * use ioctl to send mbox property message
 */

static int mbox_property(int file_desc, void *buf)
{
    int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);

    if (ret_val < 0)
    {
        // something wrong somewhere, send some details to stderr
        perror("ioctl_set_msg failed");
    }

#ifdef DEBUG
    unsigned *p = buf;
    int i;
    unsigned size = *(unsigned *)buf;
    for (i = 0; i < size / 4; i++)
        printf("%04x: 0x%08x\n", i * sizeof *p, p[i]);
#endif
    return ret_val;
}

unsigned mem_alloc(int file_desc, unsigned size, unsigned align, unsigned flags)
{
    int i = 0;
    unsigned p[32];
    p[i++] = 0;          // size
    p[i++] = 0x00000000; // process request

    p[i++] = 0x3000c; // (the tag id)
    p[i++] = 12;      // (size of the buffer)
    p[i++] = 12;      // (size of the data)
    p[i++] = size;    // (num bytes? or pages?)
    p[i++] = align;   // (alignment)
    p[i++] = flags;   // (MEM_FLAG_L1_NONALLOCATING)

    p[i++] = 0x00000000;  // end tag
    p[0] = i * sizeof *p; // actual size

    if (mbox_property(file_desc, p) < 0)
    {
        printf("mem_alloc: mbox_property() error, abort!\n");
        exit(-1);
    }
    return p[5];
}

unsigned mem_free(int file_desc, unsigned handle)
{
    int i = 0;
    unsigned p[32];
    p[i++] = 0;          // size
    p[i++] = 0x00000000; // process request

    p[i++] = 0x3000f; // (the tag id)
    p[i++] = 4;       // (size of the buffer)
    p[i++] = 4;       // (size of the data)
    p[i++] = handle;

    p[i++] = 0x00000000;  // end tag
    p[0] = i * sizeof *p; // actual size

    if (mbox_property(file_desc, p) < 0)
    {
        printf("mem_free: mbox_property() error, ignoring\n");
        return 0;
    }
    return p[5];
}

/**
 * @brief Locks memory using the mailbox interface.
 *
 * @param file_desc File descriptor for the mailbox.
 * @param handle Handle to the memory to lock.
 * @return Memory lock handle, or exits on error.
 */
unsigned mem_lock(int file_desc, unsigned handle)
{
    unsigned p[32];
    unsigned i = 0;

    p[i++] = 0;          // Size
    p[i++] = 0x00000000; // Process request
    p[i++] = 0x3000d;    // Tag ID
    p[i++] = 4;          // Size of the buffer
    p[i++] = 4;          // Size of the data
    p[i++] = handle;
    p[i++] = 0x00000000; // End tag
    p[0] = i * sizeof(*p);

    if (mbox_property(file_desc, p) < 0)
    {
        fprintf(stderr, "Error: mem_lock failed, aborting.\n");
        exit(EXIT_FAILURE);
    }

    return p[5];
}

/**
 * @brief Unlocks memory using the mailbox interface.
 *
 * @param file_desc File descriptor for the mailbox.
 * @param handle Handle to the memory to unlock.
 * @return 0 on success, or exits on error.
 */
unsigned mem_unlock(int file_desc, unsigned handle)
{
    unsigned p[32];
    unsigned i = 0;

    p[i++] = 0;          // Size
    p[i++] = 0x00000000; // Process request
    p[i++] = 0x3000e;    // Tag ID
    p[i++] = 4;          // Size of the buffer
    p[i++] = 4;          // Size of the data
    p[i++] = handle;
    p[i++] = 0x00000000; // End tag
    p[0] = i * sizeof(*p);

    if (mbox_property(file_desc, p) < 0)
    {
        fprintf(stderr, "Error: mem_unlock failed.\n");
        return 0;
    }

    return p[5];
}

/**
 * @brief Executes code in the GPU using the mailbox interface.
 *
 * @param file_desc File descriptor for the mailbox.
 * @param code Address of the code to execute.
 * @param r0-r5 Parameters to pass to the code.
 * @return Result of the code execution, or 0 on error.
 */
unsigned execute_code(int file_desc, unsigned code, unsigned r0, unsigned r1, unsigned r2, unsigned r3, unsigned r4, unsigned r5)
{
    unsigned p[32];
    unsigned i = 0;

    p[i++] = 0;          // Size
    p[i++] = 0x00000000; // Process request
    p[i++] = 0x30010;    // Tag ID
    p[i++] = 28;         // Size of the buffer
    p[i++] = 28;         // Size of the data
    p[i++] = code;
    p[i++] = r0;
    p[i++] = r1;
    p[i++] = r2;
    p[i++] = r3;
    p[i++] = r4;
    p[i++] = r5;
    p[i++] = 0x00000000; // End tag
    p[0] = i * sizeof(*p);

    if (mbox_property(file_desc, p) < 0)
    {
        fprintf(stderr, "Error: execute_code failed.\n");
        return 0;
    }

    return p[5];
}

/**
 * @brief Opens the mailbox device for communication.
 *
 * @return File descriptor for the opened mailbox device, or exits on failure.
 */
int mbox_open()
{
    int file_desc = open(DEVICE_FILE_NAME, O_RDWR);
    if (file_desc < 0)
    {
        perror("Error: Unable to open mailbox device");
        exit(EXIT_FAILURE);
    }
    return file_desc;
}

/**
 * @brief Closes the mailbox device.
 *
 * @param file_desc File descriptor for the mailbox to close.
 */
void mbox_close(int file_desc)
{
    close(file_desc);
}
