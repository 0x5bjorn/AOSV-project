#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "list.h"
#include "../module/device_shared.h"

#define UMS_DEVICE_PATH "/dev/umsdevice"
#define UMS_LIB_LOG "UMS lib: "

/* 
 * Structs
 */
typedef struct cl_list {
	struct list_head list;
	unsigned int cl_count;
} cl_list_t;

typedef struct worker_thread_list {
	struct list_head list;
	unsigned int worker_thread_count;
} worker_thread_list_t;

typedef struct ums_thread_list {
	struct list_head list;
	unsigned int ums_thread_count;
} ums_thread_list_t;

typedef struct completion_list {
	unsigned int id;
	unsigned int worker_thread_count;
    // unsigned int worker_thread_id[worker_thread_count];
	struct list_head list;
} completion_list_t;

typedef struct worker_thread {
	unsigned int id;
    worker_thread_params_t *params;
	struct list_head list;
} worker_thread_t;

typedef struct ums_thread {
	unsigned int id;
    ums_thread_params_t *params;
	struct list_head list;
} ums_thread_t;

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
completion_list_t *get_cl_with_id(unsigned int completion_list_id);
worker_thread_t *get_wt_with_id(unsigned int worker_thread_id);

__attribute__((constructor)) void constructor(void);
__attribute__((destructor)) void destructor(void);