/**
 * Copyright (C) 2021 Sultan Umarbaev <name.sul27@gmail.com>
 *
 * This file is part of UMS implementation (Kernel Module).
 *
 * UMS implementation (Kernel Module) is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UMS implementation (Kernel Module) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UMS implementation (Kernel Module).  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @brief This file is a header of the main module functionality
 *
 * This file contains all main data structures, macros, error codes 
 * and function declarations of the module
 * 
 * @file ums.h
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

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
#include "proc.h"

#define UMS_LOG "UMS: "

#define ERROR_UMS_FAIL 1							///<
#define ERROR_PROCESS_ALREADY_INITIALIZED 500		///<
#define ERROR_PROCESS_NOT_INITIALIZED 501			///<
#define ERROR_COMPLETION_LIST_NOT_FOUND 502			///<
#define ERROR_WORKER_THREAD_NOT_FOUND 503			///<
#define ERROR_UMS_THREAD_NOT_FOUND 504				///<
#define ERROR_UMS_THREAD_ALREADY_RUNNING 505		///<

/* 
 * Structs
 */

/**
 * @brief The list of processes that initialized/enabled UMS mechanism
 * 
 * The purpose of this list is to store all processes that initialized/enabled UMS mechanism
 *
 */
typedef struct process_list {
    struct list_head list;
    unsigned int process_count;		/**< The number of elements(processes) in the list */
} process_list_t;

/**
 * @brief The list of completion lists
 * 
 * The purpose of this list is to store all completion lists created by the process
 *
 */
typedef struct cl_list {
	struct list_head list;
	unsigned int cl_count;			/**< The number of elements(completion lists) in the list */
} cl_list_t;

/**
 * @brief The list of worker threads
 * 
 * The purpose of this list is to store all worker threads created by the process
 *
 */
typedef struct worker_thread_list {
	struct list_head list;
	unsigned int worker_thread_count;		/**< The number of elements(worker threads) in the list */
} worker_thread_list_t;

/**
 * @brief The list of ums threads(schedulers)
 * 
 * The purpose of this list is to store all ums threads(schedulers) created by the process
 *
 */
typedef struct ums_thread_list {
	struct list_head list;
	unsigned int ums_thread_count;		/**< The number of elements(ums threads) in the list */
} ums_thread_list_t;

/**
 * @brief The process that initialized/enabled UMS mechanism
 * 
 * This is a node in the @ref process_list. This is a process that initialized/enabled UMS mechanism.
 * Each such process has 3 lists:
 *  - list of completion lists
 *  - list of ums threads(schedulers)
 *  - list of worker threads
 *
 */
typedef struct process {
    pid_t pid;									/**< The PID of the main thread, so the TGID of every thread of the process */
    cl_list_t cl_list;							/**< A list of completion list created in this process environment */
	worker_thread_list_t worker_thread_list;	/**< A list of worker thread created in this process environment */
	ums_thread_list_t ums_thread_list;			/**< A list of ums thread(schedulers) created in this process environment */
	struct list_head list;
} process_t;

/**
 * @brief The completion list of worker threads
 * 
 * This is a node in the @ref process::cl_list. This is a description of the completion list.
 *
 */
typedef struct completion_list {
	struct list_head list;					/**< This list structure is related to the list of completion lists in the process */
	struct list_head wt_list;				/**< This list structure is related to the list of worker threads that it contains */
	unsigned int id;						/**< Unique id of the completion list */
	unsigned int worker_thread_count;		/**< The number of worker threads in this completion list */
} completion_list_t;


/**
 * @brief The state of the worker thread
 *
 */
typedef enum worker_state {
	BUSY,			/**< The worker thread is busy because it is being runned */
	READY,			/**< The worker thread is ready and can be switched to run */
    FINISHED		/**< The worker thread is finished because it has completed its task */
} worker_state_t;

typedef struct worker_thread_context {
	unsigned int id;
	struct list_head list;
	struct list_head wt_list;
	unsigned long entry_point;
	unsigned int cl_id;
	pid_t created_by;
	unsigned int run_by;
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
int init_ums_process(void);
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
process_t *get_process_with_pid(pid_t req_pid);
completion_list_t *get_cl_with_id(process_t *process, unsigned int completion_list_id);
worker_thread_context_t *get_wt_with_id(process_t *process, unsigned int worker_thread_id);
int get_ready_wt_list(completion_list_t *completion_list, unsigned int *ready_wt_list);
worker_thread_context_t *get_wt_run_by_umst_id(process_t *process, unsigned int ums_thread_id);
ums_thread_context_t *get_umst_with_id(process_t *process, unsigned int ums_thread_id);
ums_thread_context_t *get_umst_run_by_pid(process_t *process, pid_t req_pid);
int free_ums_thread(process_t *process);
int free_completion_list(process_t *process);
int free_worker_thread(process_t *process);
unsigned long get_wt_running_time(worker_thread_context_t *worker_thread_context);
