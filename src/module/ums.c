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

    process_list.process_count++;

    printk(KERN_DEBUG "UMS module: process pid = %d, INIT process count = %d\n", process->pid, process_list.process_count);

    return 0;
}

int exit_ums_process(void)
{
    int ret = 0;
    process_t *process = NULL;
    completion_list_t *completion_list = NULL;

    process = get_process_with_pid(current->tgid);
    // if (process == NULL)
    // {

    // }
    
    printk(KERN_DEBUG "UMS module: process pid = %d\n", process->pid);

    completion_list_t *temp = NULL;
    list_for_each_entry_safe(completion_list, temp, &process->cl_list.list, list) {
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

    printk(KERN_DEBUG "UMS nodule: process pid = %d, process cl_list count = %d, completion list id = %d\n", process->pid, process->cl_list.cl_count, completion_list->id);

    ret = completion_list->id;
    
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