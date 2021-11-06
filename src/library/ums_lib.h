/**
 * Copyright (C) 2021 Sultan Umarbaev <name.sul27@gmail.com>
 *
 * This file is part of UMS implementation (Library).
 *
 * UMS implementation (Library) is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UMS implementation (Library) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UMS implementation (Library).  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @brief This file is a header of the library
 *
 * This file contains all the data structures and function declarations of library
 * 
 * @file ums_lib.h
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

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

#define MIN_STACK_SIZE 4096

/* 
 * Structs
 */

/**
 * @brief The list of completion lists
 * 
 * The purpose of this list is to track and store all completion lists created by the program
 *
 */
typedef struct cl_list {
	struct list_head list;
	unsigned int cl_count;		/**< The number of elements(completion lists) in the list */
} cl_list_t;

/**
 * @brief The list of worker threads
 * 
 * The purpose of this list is to track and store all worker threads created by the program
 *
 */
typedef struct worker_thread_list {
	struct list_head list;
	unsigned int worker_thread_count;		/**< The number of elements(worker threads) in the list */
} worker_thread_list_t;

/**
 * @brief The list of ums threads(schedulers)
 * 
 * The purpose of this list is to track and store all ums threads(schedulers) created by the program
 *
 */
typedef struct ums_thread_list {
	struct list_head list;
	unsigned int ums_thread_count;		/**< The number of elements(ums threads) in the list */
} ums_thread_list_t;

/**
 * @brief The completion list of worker threads
 * 
 * This is a node in the @ref cl_list.
 *
 */
typedef struct completion_list {
	unsigned int id;						/**< Unique id of the completion list */
	unsigned int worker_thread_count;		/**< The number of elements(worker threads) in this completion list */
	struct list_head list;					
} completion_list_t;

/**
 * @brief The worker thread
 * 
 * This is a node in the @ref worker_thread_list.
 *
 */
typedef struct worker_thread {
	unsigned int id;					/**< Unique id of the worker thread */
    worker_thread_params_t *params;		/**< @see @c worker_thread_params_t*/
	struct list_head list;				
} worker_thread_t;

/**
 * @brief The ums thread(scheduler)
 * 
 * This is a node in the @ref ums_thread_list.
 *
 */
typedef struct ums_thread {
	unsigned int id;					/**< Unique id of the ums thread */
	pthread_t pt;						/**< pthread which entered ums scheduling mode */
    ums_thread_params_t *params;		/**< @see @c ums_thread_params_t*/
	struct list_head list;				
} ums_thread_t;

/* 
 * Functions
 */
int init_ums(void);
int exit_ums(void);
int create_completion_list(void);
int create_worker_thread(void (*function)(void *), void *args, unsigned long stack_size);
int add_worker_thread(unsigned int completion_list_id, unsigned int worker_thread_id);
int enter_ums_scheduling_mode(void (*function)(void *), unsigned long completion_list_id);
void *convert_to_ums_thread(void *ums_thread_id);
int exit_ums_scheduling_mode(void);
int dequeue_completion_list_items(int *ready_wt_list);
int execute_worker_thread(int *ready_wt_list, int size, unsigned int worker_thread_id);
int worker_thread_yield(yield_reason_t yield_reason);

int get_next_ready_item(int *ready_wt_list, int size);
int check_ready_wt_list(int *ready_wt_list, int size);

/* 
 * Auxiliary functions
 */
int open_dev(void);
int close_dev(void);
int get_wt_count_in_current_umst_cl(void);
completion_list_t *get_cl_with_id(unsigned int completion_list_id);
worker_thread_t *get_wt_with_id(unsigned int worker_thread_id);
ums_thread_t *get_umst_run_by_pthread(pthread_t current_pt);
int free_ums_thread_list(void);
int free_cl_list(void);
int free_worker_thread_list(void);

__attribute__((constructor)) void constructor(void);
__attribute__((destructor)) void destructor(void);