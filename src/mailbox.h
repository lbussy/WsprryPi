/**
 * @file mailbox.h
 * @brief Header file for mailbox communication with the Raspberry Pi GPU.
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

#ifndef MAILBOX_H
#define MAILBOX_H

#include <linux/ioctl.h>
#include <stdint.h>

/** New kernel version (>= 4.1) major device number. */
#define MAJOR_NUM_A 249
/** Older kernel version major device number. */
#define MAJOR_NUM_B 100
/** IOCTL command for mailbox property interface. */
#define IOCTL_MBOX_PROPERTY _IOWR(MAJOR_NUM_B, 0, char *)
/** Name of the mailbox device file. */
#define DEVICE_FILE_NAME "/dev/vcio"
/** Local fallback mailbox device file name. */
#define LOCAL_DEVICE_FILE_NAME "/tmp/mbox"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opens the mailbox device for communication.
 *
 * @return File descriptor for the mailbox device, or -1 if opening fails.
 */
int mbox_open();

/**
 * @brief Closes the mailbox device.
 *
 * @param file_desc File descriptor returned by mbox_open().
 */
void mbox_close(int file_desc);

/**
 * @brief Gets the version of the mailbox interface.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @return Version of the mailbox interface.
 */
uint32_t get_version(int file_desc);

/**
 * @brief Allocates memory using the mailbox interface.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param size Size of memory to allocate in bytes.
 * @param align Alignment of memory.
 * @param flags Flags specifying memory properties.
 * @return Handle to the allocated memory.
 */
uint32_t mem_alloc(int file_desc, uint32_t size, uint32_t align, uint32_t flags);

/**
 * @brief Frees previously allocated memory.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param handle Handle to the memory to free.
 * @return Result of the operation (0 for error, non-zero for success).
 */
uint32_t mem_free(int file_desc, uint32_t handle);

/**
 * @brief Locks allocated memory.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param handle Handle to the memory to lock.
 * @return Result of the operation (handle on success).
 */
uint32_t mem_lock(int file_desc, uint32_t handle);

/**
 * @brief Unlocks locked memory.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param handle Handle to the memory to unlock.
 * @return Result of the operation (0 for error, non-zero for success).
 */
uint32_t mem_unlock(int file_desc, uint32_t handle);

/**
 * @brief Maps physical memory into the process's address space.
 *
 * @param base Base address of the memory to map.
 * @param size Size of the memory region to map in bytes.
 * @return Pointer to the mapped memory region.
 */
void *mapmem(uint32_t base, uint32_t size);

/**
 * @brief Unmaps memory previously mapped with mapmem().
 *
 * @param addr Pointer to the mapped memory.
 * @param size Size of the mapped memory region in bytes.
 */
void unmapmem(void *addr, uint32_t size);

/**
 * @brief Executes code in the GPU using the mailbox interface.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param code Address of the code to execute.
 * @param r0-r5 Parameters passed to the GPU.
 * @return Result of the operation (0 for error, non-zero for success).
 */
uint32_t execute_code(int file_desc, uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);

/**
 * @brief Enables or disables the QPU using the mailbox interface.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param enable Non-zero to enable QPU, zero to disable.
 * @return Result of the operation (0 for error, non-zero for success).
 */
uint32_t qpu_enable(int file_desc, uint32_t enable);

/**
 * @brief Executes QPU programs using the mailbox interface.
 *
 * @param file_desc File descriptor returned by mbox_open().
 * @param num_qpus Number of QPUs to execute on.
 * @param control Control address for the QPU program.
 * @param noflush Non-zero to avoid flushing caches, zero otherwise.
 * @param timeout Timeout for execution in milliseconds.
 * @return Result of the operation (0 for error, non-zero for success).
 */
uint32_t execute_qpu(int file_desc, uint32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif // MAILBOX_H
