#pragma once

#include <asm/current.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>

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

typedef struct process {
    pid_t pid;
    cl_list_t cl_list;
	struct list_head list;
} process_t;

typedef struct completion_list {
	struct list_head list;
	unsigned int id;
} completion_list_t;

/* 
 * Functions
 */
int init_ums_process(void);
int exit_ums_process(void);
int create_completion_list(void);

/* 
 * Auxiliary functions
 */
static inline process_t *get_process_with_pid(pid_t req_pid);