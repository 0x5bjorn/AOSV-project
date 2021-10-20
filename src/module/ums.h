#pragma once

#include <asm/current.h>
#include <asm/fpu/internal.h>
#include <asm/fpu/types.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/task_stack.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

#include "device_shared.h"

#define UMS_LOG "UMS: "

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

typedef struct ums_thread_list {
	struct list_head list;
	unsigned int ums_thread_count;
} ums_thread_list_t;

typedef struct process {
    pid_t pid;
    cl_list_t cl_list;
	worker_thread_list_t worker_thread_list;
	ums_thread_list_t ums_thread_list;
	struct list_head list;
} process_t;

typedef struct completion_list {
	struct list_head list;
	struct list_head wt_list;
	unsigned int id;
	unsigned int worker_thread_count;
} completion_list_t;

typedef enum worker_state {
	BUSY,
	READY,
    FINISHED
} worker_state_t;

typedef struct worker_thread_context {
	unsigned int id;
	struct list_head list;
	struct list_head wt_list;
	unsigned long entry_point;
	pid_t created_by;
	pid_t run_by;
	worker_state_t state;
	unsigned long running_time;
	unsigned int switch_count;
	struct timespec64 last_switch_time;
	struct pt_regs regs;
	struct fpu fpu_regs;
} worker_thread_context_t;

typedef enum ums_state {
    RUNNING,
	IDLE
} ums_state_t;

typedef struct ums_thread_context {
	unsigned int id;
	struct list_head list;
	unsigned long entry_point;
	unsigned int cl_id;
	unsigned int wt_id;
	pid_t created_by;
	pid_t run_by;
	ums_state_t state;
	unsigned int switch_count;
	struct timespec64 last_switch_time;
	struct pt_regs regs;
	struct fpu fpu_regs;
	struct pt_regs ret_regs;
} ums_thread_context_t;

/* 
 * Functions
 */
int exit_ums_process(void);
int create_completion_list(void);
int create_worker_thread(worker_thread_params_t *params);
int add_to_completion_list(add_wt_params_t *params);
int create_ums_thread(ums_thread_params_t *params);
int convert_to_ums_thread(unsigned int ums_thread_id);
int convert_from_ums_thread(void);
int dequeue_completion_list_items(int *runnable_wt_ptr);
int switch_to_worker_thread(unsigned int worker_thread_id);
int switch_back_to_ums_thread(yield_reason_t yield_reason);

/* 
 * Auxiliary functions
 */
int init_ums_process(void);
process_t *get_process_with_pid(pid_t req_pid);
completion_list_t *get_cl_with_id(process_t *process, unsigned int completion_list_id);
worker_thread_context_t *get_wt_with_id(process_t *process, unsigned int worker_thread_id);
int *get_ready_wt_list(completion_list_t *completion_list, unsigned int *ready_wt_list);
worker_thread_context_t *get_wt_run_by_umst_id(process_t *process, unsigned int ums_thread_id);
ums_thread_context_t *get_umst_with_id(process_t *process, unsigned int ums_thread_id);
ums_thread_context_t *get_umst_run_by_pid(process_t *process, pid_t req_pid);
int free_ums_thread(process_t *process);
int free_completion_list(process_t *process);
int free_worker_thread(process_t *process);
unsigned long get_wt_running_time(worker_thread_context_t *worker_thread_context);