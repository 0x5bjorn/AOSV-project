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
	pthread_t pt;
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
int enter_ums_scheduling_mode(void (*function)(void *), unsigned long completion_list_id);
void *convert_to_ums_thread(void *ums_thread_id);
int exit_ums_scheduling_mode(void);
int dequeue_completion_list_items(int *ready_wt_list);
int execute_worker_thread(unsigned int worker_thread_id);
int worker_thread_yield(yield_reason_t yield_reason);

/* 
 * Auxiliary functions
 */
int open_dev(void);
int close_dev(void);
int get_wt_count_in_current_umst_cl();
completion_list_t *get_cl_with_id(unsigned int completion_list_id);
worker_thread_t *get_wt_with_id(unsigned int worker_thread_id);
ums_thread_t *get_umst_run_by_pthread(pthread_t current_pt);
int free_ums_thread(void);
int free_completion_list(void);
int free_worker_thread(void);

__attribute__((constructor)) void constructor(void);
__attribute__((destructor)) void destructor(void);