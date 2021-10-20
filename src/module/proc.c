#include "proc.h"

/*
 * Variables
 */
process_entry_list_t process_entry_list = {
    .list = LIST_HEAD_INIT(process_entry_list.list),
    .process_entry_count = 0
};

worker_thread_entry_list_t worker_thread_entry_list = {
    .list = LIST_HEAD_INIT(worker_thread_entry_list.list),
    .worker_thread_entry_count = 0
};

ums_thread_entry_list_t ums_thread_entry_list = {
    .list = LIST_HEAD_INIT(ums_thread_entry_list.list),
    .ums_thread_entry_count = 0
};

/*
 * Static decl-ns
 */
static ssize_t read_wt_entry(struct file *file, char __user *ubuf, size_t count, loff_t *offset);
static ssize_t read_umst_entry(struct file *file, char __user *ubuf, size_t count, loff_t *offset);

static struct proc_ops umst_pops = {
    .proc_read = read_wt_entry,
};

static struct proc_ops wt_pops = {
    .proc_read = read_umst_entry,
};

static struct proc_dir_entry *ums_entry;

/* 
 * Implementations
 */
int init_proc(void)
{
    printk(KERN_INFO UMS_PROC_LOG "init proc\n");
    ums_entry = proc_mkdir("ums", NULL);

    return 0;
}

void exit_proc(void)
{
    printk(KERN_INFO UMS_PROC_LOG "exit proc\n");
    proc_remove(ums_entry);
}

int create_process_entry(pid_t pid)
{
    process_entry_t *process_entry;

    process_entry = kmalloc(sizeof(process_entry_t), GFP_KERNEL);
    process_entry->pid = pid;
    list_add_tail(&process_entry->list, &process_entry_list.list);
    process_entry_list.process_entry_count++;

    char entry_name[32];
    snprintf(entry_name, 32, "%lu", (unsigned long)pid);
    process_entry->entry = proc_mkdir(entry_name, ums_entry);
    if(!process_entry->entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "Error creating /proc/ums/<PID> entry");
        return -1;
    }

    process_entry->schedulers_entry = proc_mkdir("schedulers", process_entry->entry);
    if(!process_entry->schedulers_entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "Error creating /proc/ums/<PID>/schedulers entry");
        return -1;
    }

    printk(KERN_DEBUG UMS_PROC_LOG "[INIT PROCESS ENTRY] name = %d, process entry count = %d\n", process_entry->pid, process_entry_list.process_entry_count);

    return 0;
}

int delete_process_entry(pid_t pid)
{
    process_entry_t *process_entry;
    process_entry = get_process_entry_with_pid(pid);
    if (process_entry == NULL)
    {
        return -1;
    }

    proc_remove(process_entry->schedulers_entry);
    proc_remove(process_entry->entry);
    
    list_del(&process_entry->list);
    kfree(process_entry);
    process_entry_list.process_entry_count--;

    printk(KERN_DEBUG UMS_PROC_LOG "[DELETE PROCESS ENTRY] name = %d, process entry count = %d\n", process_entry->pid, process_entry_list.process_entry_count);

    return 0;
}

/*
 * Static function impl-s
 */
// static ssize_t read_proc(struct file *file, char __user *ubuf, size_t count, loff_t *offset)
// {

// }

// static ssize_t write_proc(struct file *file, const char __user *ubuf, size_t count, loff_t *offset)
// {

// }

/* 
 * Auxiliary function impl-s
 */
process_entry_t *get_process_entry_with_pid(pid_t req_pid)
{
    if (list_empty(&process_entry_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty process entry list\n");
        return NULL;
    }

    process_entry_t *process_entry = NULL;
    process_entry_t *temp = NULL;
    list_for_each_entry_safe(process_entry, temp, &process_entry_list.list, list) {
        if (process_entry->pid == req_pid)
        {
            break;
        }
    }

    return process_entry;
}
