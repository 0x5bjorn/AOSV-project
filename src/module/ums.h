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

typedef struct process {
    pid_t pid;
	struct list_head list;
} process_t;

/* 
 * Functions
 */
int init_ums_process(void);
int exit_ums_process(void);
