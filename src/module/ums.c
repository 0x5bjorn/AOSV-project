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
 * @brief This file contains the implementation of all main functions of the module
 *
 * @file ums.c
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

#include "ums.h"

/*
 * Variables
 */
process_list_t process_list = {
    .list = LIST_HEAD_INIT(process_list.list),
    .process_count = 0
};

/* 
 * Implementations
 */

/**
 * @brief Initialize/enable UMS in the process
 *
 * First, we check if the process has already enabled UMS.
 * In order to start utilizing UMS mechanism, we need to enable UMS for the process.
 * By this we create process element in the @ref process_list which contains all processes that enbled
 * UMS mechanism.
 * If not, we create @ref process_t and initialize:
 *  - @ref process::cl_list - A list of completion list in the process environment
 *  - @ref process::worker_thread_list - A list of worker thread in the process environment
 *  - @ref process::ums_thread_list - A list of ums thread(schedulers) in the process environment
 * Additionaly, we create /proc/ums/<PID> and /proc/ums/<PID>/schedulers entries
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int init_ums_process(void)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [INIT UMS]\n");

    process_t *process;
    process = get_process_with_pid(current->tgid);
    if (process != NULL)
    {
        return -ERROR_PROCESS_ALREADY_INITIALIZED;
    }

    process = kmalloc(sizeof(process_t), GFP_KERNEL);
    list_add_tail(&process->list, &process_list.list);
    process_list.process_count++;

    process->pid = current->tgid;
    INIT_LIST_HEAD(&process->cl_list.list);
    process->cl_list.cl_count = 0;
    INIT_LIST_HEAD(&process->worker_thread_list.list);
    process->worker_thread_list.worker_thread_count = 0;
    INIT_LIST_HEAD(&process->ums_thread_list.list);
    process->ums_thread_list.ums_thread_count = 0;

    printk(KERN_DEBUG UMS_LOG "[INIT UMS] process pid = %d, process count = %d\n", process->pid, process_list.process_count);

    // proc entry
    create_process_entry(process->pid);

    return 0;
}

/**
 * @brief Delete current process that enabled UMS
 *
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Clean up memory allocated for the data structures that are associated with the process 
 * by @ref free_process() funciton.
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int exit_ums_process(void)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [EXIT UMS]\n");

    int ret = 0;
    process_t *process = NULL;
    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    
    printk(KERN_DEBUG UMS_LOG "[EXIT UMS] process pid = %d\n", process->pid);
    ret = free_process(process);
    printk(KERN_DEBUG UMS_LOG "[EXIT UMS] process count = %d\n", process_list.process_count);

    return ret;
}

/**
 * @brief Clean/free the list of processes @ref process_list
 * 
 * Delete and free each item in the list of processes @ref process_list.
 * 
 */
void exit_ums(void)
{
    process_t *process = NULL;
    process_t *temp = NULL;
    list_for_each_entry_safe(process, temp, &process_list.list, list) {
        free_process(process);
    }
}

/**
 * @brief Create completion list
 *
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary creation steps:
 * Create a new completion list and return a corresponding id. Add the completion list to 
 * @ref process::cl_list and initialize @ref completion_list::wt_list for storing worker threads.
 * Set completion_list::id to the current number of completion lists in @ref process::cl_list.
 * 
 * @return @c int completion list id
 */
int create_completion_list(void)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [CREATE CL]\n");

    int ret = 0;
    process_t *process;
    completion_list_t *completion_list;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }

    completion_list = kmalloc(sizeof(completion_list_t), GFP_KERNEL);
    completion_list->id = process->cl_list.cl_count;
    list_add_tail(&completion_list->list, &process->cl_list.list);
    process->cl_list.cl_count++;

    INIT_LIST_HEAD(&completion_list->wt_list);
    completion_list->worker_thread_count = 0;

    printk(KERN_DEBUG UMS_LOG "[CREATE CL] completion list id = %d, cl count = %d\n", completion_list->id, process->cl_list.cl_count);

    ret = completion_list->id;
    
    return ret;
}

/**
 * @brief Create worker thread
 *
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary creation steps:
 * Create a new worker thread and return a corresponding id. Add the worker thread to 
 * @ref process::worker_thread_list. Assign to structure all initial values and the ones passed by 
 * @ref params parameter, hence:
 *  - worker_thread_context::id is to the current number of worker threads in @ref process::worker_thread_list
 *  - worker_thread_context::entry_point is set to the starting function passed by @ref params::function
 *  - worker_thread_context::created_by is set to @ref process::pid
 *  - worker_thread_context::run_by is set to -1 because no scheduler is running the worker thread
 *  - worker_thread_context::state is set to @ref worker_state_t::READY
 *  - worker_thread_context::running_time is set to 0
 *  - worker_thread_context::switch_count is set to 0
 *  - worker_thread_context::regs is set to the values of @c task_pt_regs(current) function
 *  - worker_thread_context::regs::ip is set to @ref params::function, the starting function of the worker thread
 *  - worker_thread_context::regs::di is set to @ref params::function_args, the arguments to the function 
 *  - worker_thread_context::regs::sp is set to @ref params::stack_address
 *  - worker_thread_context::regs::bp is set to @ref params::stack_address
 *  - worker_thread_context::fpu_regs is set to the values of @c copy_fxregs_to_kernel() function
 * 
 * @param params pointer to data structure shared to user space to pass parameters
 * @return @c int worker thread id
 */
int create_worker_thread(worker_thread_params_t *params)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [CREATE WT]\n");

    worker_thread_params_t tmp_params;
    int ret = 0;
    process_t *process;
    worker_thread_context_t *worker_thread_context;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    worker_thread_context = kmalloc(sizeof(worker_thread_context_t), GFP_KERNEL);
    worker_thread_context->id = process->worker_thread_list.worker_thread_count;
    list_add_tail(&worker_thread_context->list, &process->worker_thread_list.list);
    process->worker_thread_list.worker_thread_count++;

    ret = copy_from_user(&tmp_params, params, sizeof(worker_thread_params_t));
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] failed to copy %d bytes of params", ret);
        return -ERROR_UMS_FAIL;
    }
    worker_thread_context->entry_point = tmp_params.function;
    worker_thread_context->created_by = process->pid;
    worker_thread_context->run_by = -1;
    worker_thread_context->state = READY;
    worker_thread_context->running_time = 0;
    worker_thread_context->switch_count = 0;

    memcpy(&worker_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
    worker_thread_context->regs.ip = tmp_params.function;
    worker_thread_context->regs.di = tmp_params.function_args;
    worker_thread_context->regs.sp = tmp_params.stack_address;
    worker_thread_context->regs.bp = tmp_params.stack_address;
    memset(&worker_thread_context->fpu_regs, 0, sizeof(struct fpu));
    copy_fxregs_to_kernel(&worker_thread_context->fpu_regs); // Legacy FPU register saving

    ret = worker_thread_context->id;

    printk(KERN_DEBUG UMS_LOG "[CREATE WT] worker thread id = %d, worker thread count = %d\n", worker_thread_context->id, process->worker_thread_list.worker_thread_count);

    return ret;
}

/**
 * @brief Add worker thread to completion list
 *
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary addition steps:
 * Check if exists and get completion list with requested @ref params::completion_list_id from @ref process::cl_list.
 * Check if exists and get worker thread with requested @ref params::worker_thread_id from @ref process::worker_thread_list.
 * Set worker_thread_context::cl_id to completion_list::id and add the worker thread to @ref completion_list::wt_list.
 * 
 * @param params pointer to data structure shared to user space to pass parameters
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int add_to_completion_list(add_wt_params_t *params)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [ADD WT TO CL]\n");

    add_wt_params_t tmp_params;
    int ret = 0;
    process_t *process;
    completion_list_t *completion_list;
    worker_thread_context_t *worker_thread_context;

    ret = copy_from_user(&tmp_params, params, sizeof(add_wt_params_t));
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] failed to copy %d bytes of params", ret);
        return -ERROR_UMS_FAIL;
    }

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    completion_list = get_cl_with_id(process, tmp_params.completion_list_id);
    if (completion_list == NULL)
    {
        return -ERROR_COMPLETION_LIST_NOT_FOUND;
    }
    worker_thread_context = get_wt_with_id(process, tmp_params.worker_thread_id);
    if (worker_thread_context == NULL)
    {
        return -ERROR_WORKER_THREAD_NOT_FOUND;
    }
    
    worker_thread_context->cl_id = tmp_params.completion_list_id;
    list_add_tail(&worker_thread_context->wt_list, &completion_list->wt_list);
    completion_list->worker_thread_count++;
    
    printk(KERN_DEBUG UMS_LOG "[ADD WT TO CL] completion list id = %d, worker thread id = %d\n", completion_list->id, worker_thread_context->id);

    return ret;
}

/**
 * @brief Create ums thread(scheduler)
 *
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary creation steps:
 * Create a new ums thread(scheduler) and add the ums thread(scheduler) to @ref process::ums_thread_list.
 * Assign to structure all initial values and the ones passed by @ref params parameter, hence:
 *  - ums_thread_context::id is to the current number of ums threads in @ref process::ums_thread_list
 *  - ums_thread_context::entry_point is set to the starting function passed by @ref params::function
 *  - ums_thread_context::cl_id is set to @ref params.completion_list_id
 *  - ums_thread_context::wt_id is set to -1, no worker thread is currently run by ums thread(scheduler)
 *  - ums_thread_context::created_by is set to @ref process::pid
 *  - ums_thread_context::run_by is set to -1, no thread is currently running the ums thread(scheduler)
 *  - ums_thread_context::state is set to @ref ums_state_t::IDLE
 *  - ums_thread_context::switch_count is set to 0
 * Additionaly, we create /proc/ums/<PID>/schedulers/<ID>, /proc/ums/<PID>/schedulers/<ID>/workers and
 * /proc/ums/<PID>/schedulers/<ID>/info entries for ums thread(scheduler).
 * As well as /proc/ums/<PID>/schedulers/<ID>/workers and /proc/ums/<PID>/schedulers/<ID>/workers/<ID>
 * entries for each worker thread in the competion list associated with ums thread(scheduler).
 *  
 * @param params pointer to data structure shared to user space to pass parameters
 * @return @c int ums thread(scheduler) id
 */
int create_ums_thread(ums_thread_params_t *params)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [CREATE UMST]\n");

    ums_thread_params_t tmp_params;
    int ret = 0;
    process_t *process;
    ums_thread_context_t *ums_thread_context;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context = kmalloc(sizeof(ums_thread_context_t), GFP_KERNEL);
    ums_thread_context->id = process->ums_thread_list.ums_thread_count;
    list_add_tail(&ums_thread_context->list, &process->ums_thread_list.list);
    process->ums_thread_list.ums_thread_count++;

    ret = copy_from_user(&tmp_params, params, sizeof(ums_thread_params_t));
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] failed to copy %d bytes of params", ret);
        return -ERROR_UMS_FAIL;
    }
    ums_thread_context->entry_point = tmp_params.function;
    ums_thread_context->cl_id = tmp_params.completion_list_id;
    ums_thread_context->wt_id = -1;
    ums_thread_context->created_by = process->pid;
    ums_thread_context->run_by = -1;
    ums_thread_context->state = IDLE;
    ums_thread_context->switch_count = 0;
    
    ret = ums_thread_context->id;

    printk(KERN_DEBUG UMS_LOG "[CREATE UMST] ums thread id = %d, umst clid = %d, ums thread count = %d\n", ums_thread_context->id, ums_thread_context->cl_id, process->ums_thread_list.ums_thread_count);

    create_umst_entry(process->pid, ums_thread_context->id);

    completion_list_t *completion_list = get_cl_with_id(process, ums_thread_context->cl_id);
    if (completion_list == NULL)
    {
        return -ERROR_COMPLETION_LIST_NOT_FOUND;
    }
    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &completion_list->wt_list, wt_list) {
        create_wt_entry(ums_thread_context->id, worker_thread_context->id);
    }

    return ret;
}

/**
 * @brief Convert thread into ums thread(scheduler)
 * 
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary convertion steps:
 * Check if exists and get ums thread(scheduler) with requested ums_thread_id from @ref process::ums_thread_list.
 * Check if this ums thread(scheduler) is IDLE and not currently run by other thread.
 * Save the context of the current thread:
 *  - ums_thread_context::run_by is set to @c current->pid
 *  - ums_thread_context::state is set to @ref ums_state_t::RUNNING
 *  - ums_thread_context::switch_count is increased by 1
 *  - ums_thread_context::last_switch_time is set to current time by @ref ktime_get_real_ts64()
 *  - ums_thread_context::regs is set to the values of @c task_pt_regs(current) function
 *  - ums_thread_context::fpu_regs is set to the values of @c copy_fxregs_to_kernel() function
 *  - ums_thread_context::ret_regs is set to @ref ums_thread_context::regs
 *  - ums_thread_context::regs::ip is set to @ref ums_thread_context::entry_point
 * Then, perform context switch operation:
 *  - switch current @c pt_regs structure to @ref ums_thread_context::regs
 * 
 * @param ums_thread_id the id of the ums thread(scheduler) to be converted to
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int convert_to_ums_thread(unsigned int ums_thread_id)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [CONVERT TO UMST]\n");

    int ret = 0;
    process_t *process;
    ums_thread_context_t *ums_thread_context;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context = get_umst_with_id(process, ums_thread_id);
    if (ums_thread_context == NULL)
    {
        return -ERROR_UMS_THREAD_NOT_FOUND;
    }

    if (ums_thread_context->state == RUNNING)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] umst is already RUNNING\n");
        return -ERROR_UMS_THREAD_ALREADY_RUNNING;
    }

    ums_thread_context->run_by = current->pid;
    ums_thread_context->state = RUNNING;
    ums_thread_context->switch_count++;
    ktime_get_real_ts64(&ums_thread_context->last_switch_time);
    memcpy(&ums_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
    memset(&ums_thread_context->fpu_regs, 0, sizeof(struct fpu));
    copy_fxregs_to_kernel(&ums_thread_context->fpu_regs); // Legacy FPU register saving
    ums_thread_context->ret_regs = ums_thread_context->regs;
    ums_thread_context->regs.ip = ums_thread_context->entry_point;

    memcpy(task_pt_regs(current), &ums_thread_context->regs, sizeof(struct pt_regs));

    printk(KERN_DEBUG UMS_LOG "[CONVERT TO UMST] ums thread id = %d, current pid = %d\n", ums_thread_context->id, current->pid);

    return ret;
}

/**
 * @brief Convert back from ums thread(scheduler)
 * 
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary convertion steps:
 * Check if exists and get ums thread(scheduler) run by @c current->pid thread from @ref process::ums_thread_list.
 * Assign to structure necessary values: 
 *  - ums_thread_context::run_by is set to -1
 *  - ums_thread_context::state is set to @ref ums_state_t::IDLE
 * Then, perform context switch operation:
 *  - switch current @c pt_regs and @c fpu structures to 
 *    @ref ums_thread_context::regs and @ref ums_thread_context::fpu_regs
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int convert_from_ums_thread(void)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [CONVERT FROM UMST]\n");

    int ret = 0;
    process_t *process;
    ums_thread_context_t *ums_thread_context;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context = get_umst_run_by_pid(process, current->pid);
    if (ums_thread_context == NULL)
    {
        return -ERROR_UMS_THREAD_NOT_FOUND;
    }

    ums_thread_context->run_by = -1;
    ums_thread_context->state = IDLE;

    memcpy(task_pt_regs(current), &ums_thread_context->ret_regs, sizeof(struct pt_regs));
    copy_kernel_to_fxregs(&ums_thread_context->fpu_regs.state.fxsave);

    printk(KERN_DEBUG UMS_LOG "[CONVERT FROM UMST] ums thread id = %d, current pid = %d\n", ums_thread_context->id, current->pid);

    return ret;
}

/**
 * @brief Dequeue completion list items
 *
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary dequeue steps:
 * Check if exists and get ums thread(scheduler) run by @c current->pid thread from @ref process::ums_thread_list.
 * Check if exists and get completion list associated with ums thread(scheduler) by @ref ums_thread_context::cl_id.
 * Allocated temporary array of integers and fill it with rnnable worker thread ids with the help
 * of auxiliary function @ref get_ready_wt_list(). Copy data from temporary array into array allocated by user.
 * 
 * @param ready_wt_list the pointer to an allocated by user array of integers which will be filled with
 * ready to run worker thread ids
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int dequeue_completion_list_items(int *read_wt_list)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [DEQUEUE CL]\n");
    
    int ret = 0;
    process_t *process;
    ums_thread_context_t *ums_thread_context;
    completion_list_t *completion_list;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context = get_umst_run_by_pid(process, current->pid);
    if (ums_thread_context == NULL)
    {
        return -ERROR_UMS_THREAD_NOT_FOUND;
    }
    completion_list = get_cl_with_id(process, ums_thread_context->cl_id);
    if (completion_list == NULL)
    {
        return -ERROR_COMPLETION_LIST_NOT_FOUND;
    }

    int list_of_wt[completion_list->worker_thread_count];
    ret = get_ready_wt_list(completion_list, list_of_wt);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] get_ready_wt_list() failed");
        return -ERROR_UMS_FAIL;
    }
    ret = copy_to_user(read_wt_list, &list_of_wt, sizeof(int)*completion_list->worker_thread_count);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] failed to copy %d bytes of params", ret);
        return -ERROR_UMS_FAIL;
    }

    int i;
    for (i = 0; i < completion_list->worker_thread_count; ++i)
    {
        printk(KERN_DEBUG UMS_LOG "[DEQUEUE CL] list_of_wt[%d] = %d\n", i, list_of_wt[i]);
        printk(KERN_DEBUG UMS_LOG "[DEQUEUE CL] ready_wt_list[%d] = %d\n", i, read_wt_list[i]);
    }
    printk(KERN_DEBUG UMS_LOG "[DEQUEUE CL] ums thread id = %d, cl id = %d, cl size = %d\n", ums_thread_context->id, completion_list->id, completion_list->worker_thread_count);
    
    return ret;
}

/**
 * @brief Switch to worker thread
 * 
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary convertion steps:
 * Check if exists and get ums thread(scheduler) run by @c current->pid thread from @ref process::ums_thread_list.
 * Check if exists and get worker thread with requested @ref params::worker_thread_id 
 * from @ref process::worker_thread_list. Check if this worker thread is not BUSY and/or FINISHED.
 * Save the context of the ums thread(scheduler):
 *  - ums_thread_context::wt_id is set to @ref worker_thread_context::id
 *  - ums_thread_context::switch_count is increased by 1
 *  - ums_thread_context::last_switch_time is set to current time by @c ktime_get_real_ts64()
 *  - ums_thread_context::regs is set to the values of @c task_pt_regs(current) function
 *  - ums_thread_context::ret_regs is set to @ref ums_thread_context::regs
 *  - ums_thread_context::fpu_regs is set to the values of @c copy_fxregs_to_kernel() function
 *  - ums_thread_context::switch_count is increased by 1
 *  - ums_thread_context::switching_time is set by an auxiliary function @ref get_umst_switching_time()
 * 
 * Then, perform context switch operation:
 *  - worker_thread_context::run_by is set to @ref ums_thread_context::id
 *  - worker_thread_context::state is set to @ref worker_state_t::BUSY
 *  - worker_thread_context::switch_count is increased by 1
 *  - worker_thread_context::last_switch_time is set to current time by @c ktime_get_real_ts64()
 *  - switch current @c pt_regs and @c fpu structures to 
 *    @ref worker_thread_context::regs and @ref worker_thread_context::fpu_regs
 * 
 * **NOTE**: If worker thread is BUSY function returns 2, if it is FINISHED it returns 1. 
 * These cases are not considered as real ERROR but handled as a special cases in userspace. 
 * In case of BUSY thread, scheduler in userspace will try to switch to next READY worker thread. 
 * For the case of FINISHED worker thread, scheduler in userpace will update list of ready worker threads.
 * 
 * @param worker_thread_id the id of the worker thread to be switched to
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int switch_to_worker_thread(unsigned int worker_thread_id)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [SWITCH TO WT]\n");

    int ret = 0;
    process_t *process;
    ums_thread_context_t *ums_thread_context;
    worker_thread_context_t *worker_thread_context;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context = get_umst_run_by_pid(process, current->pid);
    if (ums_thread_context == NULL)
    {
        return -ERROR_UMS_THREAD_NOT_FOUND;
    }
    worker_thread_context = get_wt_with_id(process, worker_thread_id);
    if (worker_thread_context == NULL)
    {
        return -ERROR_WORKER_THREAD_NOT_FOUND;
    }

    if (worker_thread_context->state == BUSY)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] wt is BUSY\n");
        return 2;
    }
    else if (worker_thread_context->state == FINISHED)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] wt is FINISHED\n");
        return 1;
    }

    ums_thread_context->wt_id = worker_thread_id;
    ums_thread_context->switch_count++;
    ktime_get_real_ts64(&ums_thread_context->last_switch_time);
    memcpy(&ums_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
    copy_fxregs_to_kernel(&ums_thread_context->fpu_regs);

    worker_thread_context->run_by = ums_thread_context->id;
    worker_thread_context->state = BUSY;
    worker_thread_context->switch_count++;
    ktime_get_real_ts64(&worker_thread_context->last_switch_time);
    memcpy(task_pt_regs(current), &worker_thread_context->regs, sizeof(struct pt_regs));
    copy_kernel_to_fxregs(&worker_thread_context->fpu_regs.state.fxsave);

    ums_thread_context->switching_time = get_umst_switching_time(ums_thread_context);
    ums_thread_context->avg_switching_time = ums_thread_context->switching_time/ums_thread_context->switch_count;

    printk(KERN_DEBUG UMS_LOG "[SWITCH TO WT] ums thread id = %d, wt id = %d\n", ums_thread_context->id, worker_thread_id);

    return ret;
}

/**
 * @brief Convert back from worker thread
 * 
 * First, we check if process that invokes this function is the one that enabled UMS.
 * Then we perform all necessary convertion steps:
 * Check if exists and get ums thread(scheduler) run by @c current->pid thread from @ref process::ums_thread_list.
 * Check if exists and get worker thread run by @ref ums_thread_context::id from @ref process::worker_thread_list. 
 * Check if the yield reason is not @ref yield_reason_t::BUSY or @ref yield_reason_t::FINISHED.
 * Then, perform context saving and context switch operations:
 *  - worker_thread_context::run_by is set to -1
 *  - worker_thread_context::state is set to @ref worker_state_t::BUSY or @ref worker_state_t::FINISHED
 *  - worker_thread_context::running_time is set by auxiiary function @ref get_wt_running_time()
 *  - worker_thread_context::regs is set to the values of @c task_pt_regs(current) function
 *  - worker_thread_context::fpu_regs is set to the values of @c copy_fxregs_to_kernel() function
 * 
 *  - ums_thread_context::wt_id is set to -1
 *  - switch current @c pt_regs and @c fpu structures to 
 *    @ref ums_thread_context::regs and @ref ums_thread_context::fpu_regs
 * 
 * @param yield_reason reason which defines if worker thread should be paused or finished, @ref yield_reason_t
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int switch_back_to_ums_thread(yield_reason_t yield_reason)
{
    printk(KERN_DEBUG UMS_LOG "--------- Invoking [SWITCH BACK TO UMST]\n");

    int ret = 0;
    process_t *process;
    ums_thread_context_t *ums_thread_context;
    worker_thread_context_t *worker_thread_context;

    process = get_process_with_pid(current->tgid);
    if (process == NULL)
    {
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context = get_umst_run_by_pid(process, current->pid);
    if (ums_thread_context == NULL)
    {
        return -ERROR_UMS_THREAD_NOT_FOUND;
    }
    worker_thread_context = get_wt_run_by_umst_id(process, ums_thread_context->id);
    if (worker_thread_context == NULL)
    {
        return -ERROR_WORKER_THREAD_NOT_FOUND;
    }

    if (yield_reason == FINISH)
    {
        worker_thread_context->run_by = -1;
        worker_thread_context->state = FINISHED;
        worker_thread_context->running_time = get_wt_running_time(worker_thread_context);
        memcpy(&worker_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
        copy_fxregs_to_kernel(&worker_thread_context->fpu_regs);

        ums_thread_context->wt_id = -1;
        memcpy(task_pt_regs(current), &ums_thread_context->regs, sizeof(struct pt_regs));
        copy_kernel_to_fxregs(&ums_thread_context->fpu_regs.state.fxsave);
    }
    else if (yield_reason == PAUSE)
    {
        worker_thread_context->run_by = -1;
        worker_thread_context->state = READY;
        worker_thread_context->running_time = get_wt_running_time(worker_thread_context);
        memcpy(&worker_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
        copy_fxregs_to_kernel(&worker_thread_context->fpu_regs);

        ums_thread_context->wt_id = -1;
        memcpy(task_pt_regs(current), &ums_thread_context->regs, sizeof(struct pt_regs));
        copy_kernel_to_fxregs(&ums_thread_context->fpu_regs.state.fxsave);
    }

    printk(KERN_DEBUG UMS_LOG "[SWITCH BACK TO UMST] ums thread id = %d, wt id = %d, yield reason = %d\n", ums_thread_context->id, worker_thread_context->id, yield_reason);

    return ret;
}

/* 
 * Auxiliary function impl-s
 */

/**
 * @brief Get process structure from @ref process_list with specific PID
 * 
 * @param req_pid the PID of the current process
 * @return @c process_t the pointer to process structure
 */
process_t *get_process_with_pid(pid_t req_pid)
{
    if (list_empty(&process_list.list))
    {
        return NULL;
    }

    process_t *result = NULL;
    process_t *process = NULL;
    process_t *temp = NULL;
    list_for_each_entry_safe(process, temp, &process_list.list, list) {
        if (process->pid == req_pid)
        {
            result = process;
            break;
        }
    }

    return result;
}

/**
 * @brief Get completion list from @ref process::cl_list with specific id
 * 
 * @param process the pointer to the process structure of the current process
 * @param completion_list_id the id of the completion list requested to be retrieved
 * @return @c completion_list_t the pointer to completion list
 */
completion_list_t *get_cl_with_id(process_t *process, unsigned int completion_list_id)
{
    if (list_empty(&process->cl_list.list))
    {
        return NULL;
    }

    completion_list_t *result = NULL;
    completion_list_t *completion_list = NULL;
    completion_list_t *temp = NULL;
    list_for_each_entry_safe(completion_list, temp, &process->cl_list.list, list) {
        if (completion_list->id == completion_list_id)
        {
            result = completion_list;
            break;
        }
    }

    return result;
}

/**
 * @brief Get worker thread from @ref process::worker_thread_list with specific id
 * 
 * @param process the pointer to the process structure of the current process
 * @param worker_thread_id the id of the worker thread requested to be retrieved
 * @return @c worker_thread_context_t the pointer to worker thread
 */
worker_thread_context_t *get_wt_with_id(process_t *process, unsigned int worker_thread_id)
{
    if (list_empty(&process->worker_thread_list.list))
    {
        return NULL;
    }

    worker_thread_context_t *result = NULL;
    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &process->worker_thread_list.list, list) {
        if (worker_thread_context->id == worker_thread_id)
        {
            result = worker_thread_context;
            break;
        }
    }

    return result;
}

/**
 * @brief Fill the array of integers with ready worker thread ids from completion list
 * 
 * @param completion_list the pointer to the completion list from which to get worker thread ids
 * @param ready_wt_list the pointer to the array of integers that will be filled
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int get_ready_wt_list(completion_list_t *completion_list, unsigned int *ready_wt_list)
{
    if (list_empty(&completion_list->wt_list))
    {
        return -1;
    }

    int i = 0;
    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &completion_list->wt_list, wt_list) {
        if (worker_thread_context->state == READY)
        {
            ready_wt_list[i] = worker_thread_context->id;
        }
        else
        {
            ready_wt_list[i] = -1;
        }
        i++;
    }

    return 0;
}

/**
 * @brief Get worker thread from @ref process::worker_thread_list run by specific ums thread(scheduler)
 * 
 * @param process the pointer to the process structure of the current process
 * @param ums_thread_id the id of the ums thread(scheduler) that runs worker thread
 * @return @c worker_thread_context_t the pointer to worker thread
 */
worker_thread_context_t *get_wt_run_by_umst_id(process_t *process, unsigned int ums_thread_id)
{
    if (list_empty(&process->worker_thread_list.list))
    {
        return NULL;
    }

    worker_thread_context_t *result = NULL;
    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &process->worker_thread_list.list, list) {
        if (worker_thread_context->run_by == ums_thread_id)
        {
            result = worker_thread_context;
            break;
        }
    }

    return result;
}

/**
 * @brief Get ums thread(scheduler) from @ref process::ums_thread_list with specific id
 * 
 * @param process the pointer to the process structure of the current process
 * @param ums_thread_id the id of the ums thread(scheduler) requested to be retrieved
 * @return @c ums_thread_context_t the pointer to ums thread(scheduler)
 */
ums_thread_context_t *get_umst_with_id(process_t *process, unsigned int ums_thread_id)
{
    if (list_empty(&process->ums_thread_list.list))
    {
        return NULL;
    }

    ums_thread_context_t *result = NULL;
    ums_thread_context_t *ums_thread_context = NULL;
    ums_thread_context_t *temp = NULL;
    list_for_each_entry_safe(ums_thread_context, temp, &process->ums_thread_list.list, list) {
        if (ums_thread_context->id == ums_thread_id)
        {
            result = ums_thread_context;
            break;
        }
    }

    return result;
}

/**
 * @brief Get ums thread(scheduler) from @ref process::ums_thread_list run by specific thread
 * 
 * @param process the pointer to the process structure of the current process
 * @param req_pid the PID of the thread that runs ums thread(scheduler)
 * @return @c ums_thread_context_t the pointer to ums thread(scheduler)
 */
ums_thread_context_t *get_umst_run_by_pid(process_t *process, pid_t req_pid)
{
    if (list_empty(&process->ums_thread_list.list))
    {
        return NULL;
    }

    ums_thread_context_t *result = NULL;
    ums_thread_context_t *ums_thread_context = NULL;
    ums_thread_context_t *temp = NULL;
    list_for_each_entry_safe(ums_thread_context, temp, &process->ums_thread_list.list, list) {
        if (ums_thread_context->run_by == req_pid)
        {
            result = ums_thread_context;
            break;
        }
    }

    return result;
}

/**
 * @brief Clean the list of ums threads(schedulers)
 * 
 * Delete and free each item in the list of ums threads(schedulers)
 * 
 * @param process the pointer to the process structure of the current process
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_process_ums_thread_list(process_t *process)
{
    if (list_empty(&process->ums_thread_list.list))
    {
        return -1;
    }

    ums_thread_context_t *ums_thread_context = NULL;
    ums_thread_context_t *temp = NULL;
    list_for_each_entry_safe(ums_thread_context, temp, &process->ums_thread_list.list, list) {
        printk(KERN_DEBUG UMS_LOG "[DELETE UMST] ums thread id = %d\n", ums_thread_context->id);
        list_del(&ums_thread_context->list);
        kfree(ums_thread_context);
        process->ums_thread_list.ums_thread_count--;
        printk(KERN_DEBUG UMS_LOG "[DELETE UMST] process umst count = %d\n", process->ums_thread_list.ums_thread_count);
    }

    return 0;
}

/**
 * @brief Clean the list of completion lists
 * 
 * Delete and free each item in the list of completion lists
 * 
 * @param process the pointer to the process structure of the current process
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_process_cl_list(process_t *process)
{
    if (list_empty(&process->cl_list.list))
    {
        return -1;
    }

    completion_list_t *completion_list = NULL;
    completion_list_t *temp = NULL;
    list_for_each_entry_safe(completion_list, temp, &process->cl_list.list, list) {
        printk(KERN_DEBUG UMS_LOG "[DELETE CL] completion list id = %d\n", completion_list->id);
        list_del(&completion_list->list);
        kfree(completion_list);
        process->cl_list.cl_count--;
        printk(KERN_DEBUG UMS_LOG "[DELETE CL] process cl count = %d\n", process->cl_list.cl_count);
    }

    return 0;
}

/**
 * @brief Clean the list of worker threads
 * 
 * Delete and free each item in the list of worker threads
 * 
 * @param process the pointer to the process structure of the current process
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_process_worker_thread_list(process_t *process)
{
    if (list_empty(&process->worker_thread_list.list))
    {
        return -1;
    }

    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &process->worker_thread_list.list, list) {
        printk(KERN_DEBUG UMS_LOG "[DELETE WT] worker thread id = %d\n", worker_thread_context->id);
        list_del(&worker_thread_context->wt_list);
        list_del(&worker_thread_context->list);
        kfree(worker_thread_context);
        process->worker_thread_list.worker_thread_count--;
        printk(KERN_DEBUG UMS_LOG "[DELETE WT] process worker thread count = %d\n", process->worker_thread_list.worker_thread_count);
    }

    return 0;
}

/**
 * @brief Delete/free process structure
 * 
 * Delete and free specific process. In particular, delete every element in:
 *  - @ref process::cl_list
 *  - @ref process::worker_thread_list
 *  - @ref process::ums_thread_list
 * And after that delete the process from @ref process_list, the list of all processes that enabled UMS.
 * 
 * @param process the pointer to the process structure of the process to be deleted
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_process(process_t *process)
{
    int ret = 0;

    ret = free_process_worker_thread_list(process);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] free_process_worker_thread_list() failed", ret);
        return -ERROR_UMS_FAIL;
    }
    ret = free_process_cl_list(process);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] free_process_cl_list() failed", ret);
        return -ERROR_UMS_FAIL;
    }
    ret = free_process_ums_thread_list(process);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] free_process_ums_thread_list() failed", ret);
        return -ERROR_UMS_FAIL;
    }

    list_del(&process->list);
    kfree(process);
    process_list.process_count--;

    return ret;
}

/**
 * @brief Calculate the total runnning time of the worker thread
 *
 * @param worker_thread_context the pointer to the worker thread which running time to be calculated
 * @return @c unsigned @c long calculated running time
 */
unsigned long get_wt_running_time(worker_thread_context_t *worker_thread_context)
{
    struct timespec64 current_timespec;
    unsigned long current_time;
    unsigned long worker_thread_time;
    unsigned long running_time;

    ktime_get_real_ts64(&current_timespec);
    current_time = current_timespec.tv_sec * 1000000000 + current_timespec.tv_nsec;
    worker_thread_time = worker_thread_context->last_switch_time.tv_sec * 1000000000 + worker_thread_context->last_switch_time.tv_nsec;
    running_time = worker_thread_context->running_time + (current_time - worker_thread_time);

    return running_time;
}

/**
 * @brief Calculate the total switching time of the ums thread
 *
 * @param ums_thread_context the pointer to the ums thread which swithcing time to be calculated
 * @return @c unsigned @c long calculated total switching time
 */
unsigned long get_umst_switching_time(ums_thread_context_t *ums_thread_context)
{
    struct timespec64 current_timespec;
    unsigned long current_time;
    unsigned long ums_thread_time;
    unsigned long switching_time;

    ktime_get_real_ts64(&current_timespec);
    current_time = current_timespec.tv_sec * 1000000000 + current_timespec.tv_nsec;
    ums_thread_time = ums_thread_context->last_switch_time.tv_sec * 1000000000 + ums_thread_context->last_switch_time.tv_nsec;
    switching_time = ums_thread_context->switching_time + (current_time - ums_thread_time);

    return switching_time;
}
