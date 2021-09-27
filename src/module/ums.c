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
    process_t *process;

    process = kmalloc(sizeof(process_t), GFP_KERNEL);
    list_add_tail(&process->list, &process_list.list);

    process->pid = current->tgid;
    INIT_LIST_HEAD(&process->cl_list.list);
    process->cl_list.cl_count = 0;
    INIT_LIST_HEAD(&process->worker_thread_list.list);
    process->worker_thread_list.worker_thread_count = 0;

    process_list.process_count++;

    printk(KERN_DEBUG "UMS module: process pid = %d, INIT process count = %d\n", process->pid, process_list.process_count);

    return 0;
}

int exit_ums_process(void)
{
    int ret = 0;
    process_t *process = NULL;
    completion_list_t *completion_list = NULL;
    worker_thread_context_t *worker_thread_context = NULL;

    process = get_process_with_pid(current->tgid);
    // if (process == NULL)
    // {

    // }
    
    printk(KERN_DEBUG "UMS module: process pid = %d\n", process->pid);

    worker_thread_context_t *temp_wt = NULL;
    list_for_each_entry_safe(worker_thread_context, temp_wt, &process->worker_thread_list.list, list) {
        printk(KERN_DEBUG "UMS nodule: worker thread id = %d\n", worker_thread_context->id);
        list_del(&worker_thread_context->list);
        kfree(worker_thread_context);
        process->worker_thread_list.worker_thread_count--;
        printk(KERN_DEBUG "UMS nodule: process wt_list count = %d\n", process->worker_thread_list.worker_thread_count);
    }

    completion_list_t *temp_cl = NULL;
    list_for_each_entry_safe(completion_list, temp_cl, &process->cl_list.list, list) {
        printk(KERN_DEBUG "UMS nodule: completion list id = %d\n", completion_list->id);
        list_del(&completion_list->list);
        kfree(completion_list);
        process->cl_list.cl_count--;
        printk(KERN_DEBUG "UMS nodule: process cl_list count = %d\n", process->cl_list.cl_count);
    }

    list_del(&process->list);
    kfree(process);
    process_list.process_count--;

    printk(KERN_DEBUG "UMS module: EXIT process count = %d\n", process_list.process_count);

    return ret;
}

int create_completion_list(void)
{
    int ret;
    process_t *process;
    completion_list_t *completion_list;

    process = get_process_with_pid(current->tgid);
    completion_list = kmalloc(sizeof(completion_list_t), GFP_KERNEL);
    list_add_tail(&completion_list->list, &process->cl_list.list);
    completion_list->id = process->cl_list.cl_count;
    process->cl_list.cl_count++;

    INIT_LIST_HEAD(&completion_list->wt_list);
    completion_list->worker_thread_count = 0;

    printk(KERN_DEBUG "UMS nodule: process pid = %d, process cl_list count = %d, completion list id = %d\n", process->pid, process->cl_list.cl_count, completion_list->id);

    ret = completion_list->id;
    
    return ret;
}

int create_worker_thread(worker_thread_params_t *params)
{
    worker_thread_params_t tmp_params;
    int ret;
    process_t *process;
    worker_thread_context_t *worker_thread_context;

    process = get_process_with_pid(current->tgid);
    worker_thread_context = kmalloc(sizeof(worker_thread_context_t), GFP_KERNEL);
    list_add_tail(&worker_thread_context->list, &process->worker_thread_list.list);
    worker_thread_context->id = process->worker_thread_list.worker_thread_count;
    process->worker_thread_list.worker_thread_count++;

    copy_from_user(&tmp_params, params, sizeof(worker_thread_params_t));
    worker_thread_context->entry_point = tmp_params.function;
    memcpy(&worker_thread_context->regs, task_pt_regs(current), sizeof(struct pt_regs));
    worker_thread_context->regs.ip = tmp_params.function;
    worker_thread_context->regs.di = tmp_params.function_args;
    worker_thread_context->regs.sp = tmp_params.stack_address;
    worker_thread_context->regs.bp = tmp_params.stack_address;
    worker_thread_context->stack_size = tmp_params.stack_size; //*
    memset(&worker_thread_context->fpu_regs, 0, sizeof(struct fpu));
    copy_fxregs_to_kernel(&worker_thread_context->fpu_regs); // Legacy FPU register saving
    worker_thread_context->created_by = process->pid;
    worker_thread_context->run_by = -1;
    worker_thread_context->state = IDLE;
    worker_thread_context->running_time = 0;
    worker_thread_context->switch_count = 0;

    ret = worker_thread_context->id;

    printk(KERN_DEBUG "UMS nodule: process pid = %d, process wt_list count = %d, worker thread id = %d\n", process->pid, process->worker_thread_list.worker_thread_count, worker_thread_context->id);
    printk(KERN_DEBUG "UMS nodule: worker thread stack_size = %d\n", worker_thread_context->stack_size);

    return ret;
}

/* 
 * Auxiliary function impl-s
 */
static inline process_t *get_process_with_pid(pid_t req_pid)
{
    // if (list_empty(&process_list.list))
    // {

    // }

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