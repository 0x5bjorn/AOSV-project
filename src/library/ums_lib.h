#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>

#include "../module/device_shared.h"

#define UMS_DEVICE_PATH "/dev/umsdevice"

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
