#pragma once

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ums.h"

#define UMS_PROC_LOG "UMS proc: "

#define PROC_BUFFER_SIZE 100

/* 
 * Structs
 */
typedef struct process_entry_list {
    struct list_head list;
    unsigned int process_entry_count;
} process_entry_list_t;

typedef struct ums_thread_entry_list {
	struct list_head list;
	unsigned int ums_thread_entry_count;
} ums_thread_entry_list_t;

typedef struct worker_thread_entry_list {
	struct list_head list;
	unsigned int worker_thread_entry_count;
} worker_thread_entry_list_t;

typedef struct process_entry {
    pid_t pid;
	struct list_head list;
	struct proc_dir_entry *entry;
    struct proc_dir_entry *schedulers_entry;
} process_entry_t;

typedef struct ums_thread_entry {
	unsigned int id;
	struct list_head list;
	struct proc_dir_entry *entry;
    struct proc_dir_entry *workers_entry;
    struct proc_dir_entry *info_entry;
} ums_thread_entry_t;

typedef struct worker_thread_entry {
	unsigned int id;
	struct list_head list;
	struct proc_dir_entry *entry;
} worker_thread_entry_t;

/*
 * Functions
 */
int init_proc(void);
void exit_proc(void);
int create_process_entry(pid_t pid);
int create_umst_entry(pid_t pid, unsigned int umst_id);
int create_wt_entry(unsigned int umst_id, unsigned int wt_id);

/* 
 * Auxiliary functions
 */
process_entry_t *get_process_entry_with_pid(pid_t req_pid);
ums_thread_entry_t *get_ums_thread_entry_with_pid(unsigned int id);
