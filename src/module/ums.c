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

    process_list.process_count++;

    printk(KERN_DEBUG "UMS device: process pid = %d\n", process->pid);
    printk(KERN_DEBUG "UMS device: INIT process count = %d\n", process_list.process_count);

    return 0;
}

int exit_ums_process(void)
{
    int ret = 0;
    process_t *process = NULL;
 
    process_t *temp = NULL;
    list_for_each_entry_safe(process, temp, &process_list.list, list) {
        if (process->pid == current->tgid)
        {
            break;
        }
    }

    printk(KERN_DEBUG "UMS device: process pid = %d\n", process->pid);

    list_del(&process->list);
    process_list.process_count--;
    kfree(process);

    printk(KERN_DEBUG "UMS device: EXIT process count = %d\n", process_list.process_count);

    return ret;
}