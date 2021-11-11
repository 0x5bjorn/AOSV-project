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
 * @brief This file contains definitions of ioctl commands
 *
 * This file is also included in the library
 * Defined ioctl commands are mapped with the library functions:
 * - UMS_DEV_INIT_UMS_PROCESS           <-> init_ums()
 * - UMS_DEV_EXIT_UMS_PROCESS           <-> exit_ums()
 * - UMS_DEV_CREATE_COMPLETION_LIST     <-> create_completion_list()
 * - UMS_DEV_CREATE_WORKER_THREAD       <-> create_worker_thread()
 * - UMS_DEV_ADD_TO_COMPLETION_LIST     <-> add_worker_thread()
 * - UMS_DEV_CREATE_UMS_THREAD          <-> enter_ums_scheduling_mode()
 * - UMS_DEV_CONVERT_TO_UMS_THREAD      <-> convert_to_ums_thread()
 * - UMS_DEV_CONVERT_FROM_UMS_THREAD    <-> exit_ums_scheduling_mode()
 * - UMS_DEV_DEQUEUE_CL_ITEMS           <-> dequeue_completion_list_items()
 * - UMS_DEV_SWITCH_TO_WORKER_THREAD    <-> execute_worker_thread()
 * - UMS_DEV_SWITCH_BACK_TO_UMS_THREAD  <-> worker_thread_yield()
 *
 * @file device_shared.h
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

#pragma once

#include <linux/ioctl.h>

#define UMS_DEV_IOCTL_MAGIC 'R'

/**
 * @brief Parameters passed by the library and user for worker thread creation
 *
 */
typedef struct worker_thread_params {
    unsigned long function;             /**< The pointer to starting function of the worker thread passed by the user */
    unsigned long function_args;        /**< The pointer to arguments for function */
    unsigned long stack_address;        /**< The starting address of the stack allocated by the library */
    unsigned long stack_size;           /**< The size of the allocated stack */
} worker_thread_params_t;

/**
 * @brief Parameters passed by the library and user for adding worker thread to completion list
 *
 */
typedef struct add_wt_params {
    unsigned int completion_list_id;    /**< The id of the completion list to which is added */
    unsigned int worker_thread_id;      /**< The id of the worker thread which is being added */
} add_wt_params_t;

/**
 * @brief Parameters passed by the library and user for ums thread(scheduler) creation
 *
 */
typedef struct ums_thread_params {
    unsigned long function;             /**< The pointer to starting function of the ums thread(scheduler) passed by the user */
    unsigned int completion_list_id;    /**< The id of the completion list associated to ums thread(scheduler) */
} ums_thread_params_t;

/**
 * @brief Parameters passed by the library and user for worker thread yield function
 *
 */
typedef enum yield_reason {
    FINISH,         /**< The yield reason is FINSIH when worker thread has finished its task */
	PAUSE           /**< The yield reason is PAUSE when worker thread hasn't finished its task but for some
                         reason is requested to be blocked/paused until next invokation */
} yield_reason_t;

#define UMS_DEV_INIT_UMS_PROCESS            _IO(UMS_DEV_IOCTL_MAGIC, 0)
#define UMS_DEV_EXIT_UMS_PROCESS            _IO(UMS_DEV_IOCTL_MAGIC, 1)
#define UMS_DEV_CREATE_COMPLETION_LIST      _IO(UMS_DEV_IOCTL_MAGIC, 2)
#define UMS_DEV_CREATE_WORKER_THREAD        _IOW(UMS_DEV_IOCTL_MAGIC, 3, worker_thread_params_t *)
#define UMS_DEV_ADD_TO_COMPLETION_LIST      _IOW(UMS_DEV_IOCTL_MAGIC, 4, add_wt_params_t *)
#define UMS_DEV_CREATE_UMS_THREAD           _IOW(UMS_DEV_IOCTL_MAGIC, 5, ums_thread_params_t *)
#define UMS_DEV_CONVERT_TO_UMS_THREAD       _IOW(UMS_DEV_IOCTL_MAGIC, 6, int)
#define UMS_DEV_CONVERT_FROM_UMS_THREAD     _IO(UMS_DEV_IOCTL_MAGIC, 7)
#define UMS_DEV_DEQUEUE_CL_ITEMS            _IOWR(UMS_DEV_IOCTL_MAGIC, 8, int *)
#define UMS_DEV_SWITCH_TO_WORKER_THREAD     _IOW(UMS_DEV_IOCTL_MAGIC, 9, int)
#define UMS_DEV_SWITCH_BACK_TO_UMS_THREAD   _IOW(UMS_DEV_IOCTL_MAGIC, 10, yield_reason_t)
