#pragma once

#include <asm/current.h>
#include <asm/fpu/internal.h>
#include <asm/fpu/types.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/sched/task_stack.h>

#include "device_shared.h"

/* 
 * Structs
 */
typedef struct process_list {
    struct list_head list;
    unsigned int process_count;
} process_list_t;

typedef struct cl_list {
	struct list_head list;
	unsigned int cl_count;
} cl_list_t;

typedef struct worker_thread_list {
	struct list_head list;
	unsigned int worker_thread_count;
} worker_thread_list_t;

typedef struct process {
    pid_t pid;
    cl_list_t cl_list;
	worker_thread_list_t worker_thread_list;
	struct list_head list;
} process_t;

typedef struct completion_list {
	struct list_head list;
	struct list_head wt_list;
	unsigned int id;
	unsigned int worker_thread_count;
} completion_list_t;

typedef enum worker_state {
    RUNNING,
	IDLE,
    FINISHED
} worker_state_t;

typedef struct worker_thread_context {
	unsigned int id;
	struct pt_regs regs;
	struct fpu fpu_regs;
	struct list_head list;
	struct list_head wt_list;
	unsigned long entry_point;
	pid_t created_by;
	pid_t run_by;
	worker_state_t state;
	unsigned long stack_size;
	unsigned long running_time;
	unsigned int switch_count;
} worker_thread_context_t;

/* 
 * Functions
 */
int init_ums_process(void);
int exit_ums_process(void);
int create_completion_list(void);
int create_worker_thread(worker_thread_params_t *params);

/* 
 * Auxiliary functions
 */
static inline process_t *get_process_with_pid(pid_t req_pid);
