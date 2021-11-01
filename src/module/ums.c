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
    ret = free_worker_thread(process);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] free_worker_thread() failed", ret);
        return -ERROR_UMS_FAIL;
    }
    ret = free_completion_list(process);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] free_completion_list() failed", ret);
        return -ERROR_UMS_FAIL;
    }
    ret = free_ums_thread(process);
    if (ret != 0)
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] free_ums_thread() failed", ret);
        return -ERROR_UMS_FAIL;
    }

    list_del(&process->list);
    kfree(process);
    process_list.process_count--;
    printk(KERN_DEBUG UMS_LOG "[EXIT UMS] process count = %d\n", process_list.process_count);

    return ret;
}

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
        printk(KERN_DEBUG UMS_LOG "umst id = %d, cl id = %d, wt id = %d\n", ums_thread_context->id, ums_thread_context->cl_id, worker_thread_context->id);
        create_wt_entry(ums_thread_context->id, worker_thread_context->id);
    }

    return ret;
}

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

int convert_from_ums_thread()
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
    memcpy(&ums_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
    copy_fxregs_to_kernel(&ums_thread_context->fpu_regs);

    worker_thread_context->run_by = ums_thread_context->id;
    worker_thread_context->state = BUSY;
    worker_thread_context->switch_count++;
    ktime_get_real_ts64(&worker_thread_context->last_switch_time);
    memcpy(task_pt_regs(current), &worker_thread_context->regs, sizeof(struct pt_regs));
    copy_kernel_to_fxregs(&worker_thread_context->fpu_regs.state.fxsave);

    printk(KERN_DEBUG UMS_LOG "[SWITCH TO WT] ums thread id = %d, wt id = %d\n", ums_thread_context->id, worker_thread_id);

    return ret;
}

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
        ums_thread_context->switch_count++;
        ktime_get_real_ts64(&ums_thread_context->last_switch_time);
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
        ums_thread_context->switch_count++;
        ktime_get_real_ts64(&ums_thread_context->last_switch_time);
        memcpy(task_pt_regs(current), &ums_thread_context->regs, sizeof(struct pt_regs));
        copy_kernel_to_fxregs(&ums_thread_context->fpu_regs.state.fxsave);
    }

    printk(KERN_DEBUG UMS_LOG "[SWITCH BACK TO UMST] ums thread id = %d, wt id = %d, yield reason = %d\n", ums_thread_context->id, worker_thread_context->id, yield_reason);

    return ret;
}

/* 
 * Auxiliary function impl-s
 */
process_t *get_process_with_pid(pid_t req_pid)
{
    if (list_empty(&process_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty process list\n");
        return NULL;
    }

    process_t *process = NULL;
    process_t *temp = NULL;
    list_for_each_entry_safe(process, temp, &process_list.list, list) {
        if (process->pid == req_pid)
        {
            break;
        }
    }

    return process;
}

completion_list_t *get_cl_with_id(process_t *process, unsigned int completion_list_id)
{
    if (list_empty(&process->cl_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty cl list\n");
        return NULL;
    }

    completion_list_t *completion_list = NULL;
    completion_list_t *temp = NULL;
    list_for_each_entry_safe(completion_list, temp, &process->cl_list.list, list) {
        if (completion_list->id == completion_list_id)
        {
            break;
        }
    }

    return completion_list;
}

worker_thread_context_t *get_wt_with_id(process_t *process, unsigned int worker_thread_id)
{
    if (list_empty(&process->worker_thread_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty wt list\n");
        return NULL;
    }

    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &process->worker_thread_list.list, list) {
        if (worker_thread_context->id == worker_thread_id)
        {
            break;
        }
    }

    return worker_thread_context;
}

int get_ready_wt_list(completion_list_t *completion_list, unsigned int *ready_wt_list)
{
    if (list_empty(&completion_list->wt_list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] No worker threads in completion list\n");
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

worker_thread_context_t *get_wt_run_by_umst_id(process_t *process, unsigned int ums_thread_id)
{
    if (list_empty(&process->worker_thread_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty wt list\n");
        return NULL;
    }

    worker_thread_context_t *worker_thread_context = NULL;
    worker_thread_context_t *temp = NULL;
    list_for_each_entry_safe(worker_thread_context, temp, &process->worker_thread_list.list, list) {
        if (worker_thread_context->run_by == ums_thread_id)
        {
            break;
        }
    }

    return worker_thread_context;
}

ums_thread_context_t *get_umst_with_id(process_t *process, unsigned int ums_thread_id)
{
    if (list_empty(&process->ums_thread_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty umst list\n");
        return NULL;
    }

    ums_thread_context_t *ums_thread_context = NULL;
    ums_thread_context_t *temp = NULL;
    list_for_each_entry_safe(ums_thread_context, temp, &process->ums_thread_list.list, list) {
        if (ums_thread_context->id == ums_thread_id)
        {
            break;
        }
    }

    return ums_thread_context;
}

ums_thread_context_t *get_umst_run_by_pid(process_t *process, pid_t req_pid)
{
    if (list_empty(&process->ums_thread_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty umst list\n");
        return NULL;
    }

    ums_thread_context_t *ums_thread_context = NULL;
    ums_thread_context_t *temp = NULL;
    list_for_each_entry_safe(ums_thread_context, temp, &process->ums_thread_list.list, list) {
        if (ums_thread_context->run_by == req_pid)
        {
            break;
        }
    }

    return ums_thread_context;
}

int free_ums_thread(process_t *process)
{
    if (list_empty(&process->ums_thread_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty umst list\n");
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

int free_completion_list(process_t *process)
{
    if (list_empty(&process->cl_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty cl list\n");
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

int free_worker_thread(process_t *process)
{
    if (list_empty(&process->worker_thread_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty wt list\n");
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

unsigned long get_wt_running_time(worker_thread_context_t *worker_thread_context)
{
    struct timespec64 current_timespec;
    unsigned long current_time;
    unsigned long worker_thread_time;
    unsigned long running_time;

    ktime_get_real_ts64(&current_timespec);
    current_time = current_timespec.tv_sec * 1000 + current_timespec.tv_nsec / 1000000;
    worker_thread_time = worker_thread_context->last_switch_time.tv_sec * 1000 + worker_thread_context->last_switch_time.tv_nsec / 1000000;
    running_time = worker_thread_context->running_time + (current_time - worker_thread_time);

    return running_time;
}