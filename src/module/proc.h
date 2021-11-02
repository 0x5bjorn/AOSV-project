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
 * @brief This file is a header of the /proc part of the module
 *
 * This file contains all data structures, macros, error codes 
 * and function declarations of the /proc part
 * 
 * @file proc.h
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

#pragma once

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ums.h"

#define UMS_PROC_LOG "UMS proc: "

#define ERROR_PROC_FAIL 1
#define ERROR_PROCESS_ENTRY_NOT_FOUND 601
#define ERROR_UMST_ENTRY_NOT_FOUND 602

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
