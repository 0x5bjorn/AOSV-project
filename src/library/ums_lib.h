#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "../module/device_shared.h"

#define UMS_DEVICE_PATH "/dev/umsdevice"
#define UMS_LIB_LOG "UMS lib: "

/* 
 * Structs
 */


/* 
 * Functions
 */
int init_ums();
int exit_ums();
int create_completion_list();
int create_worker_thread(void (*function)(void *), void *args, unsigned long stack_size);
int add_worker_thread(unsigned int completion_list_id, unsigned int worker_thread_id);
int create_ums_thread(void (*function)(void *), unsigned long completion_list_id);

/* 
 * Auxiliary functions
 */
int open_dev(void);

__attribute__((constructor)) void constructor(void);
__attribute__((destructor)) void destructor(void);