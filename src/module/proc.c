#include "proc.h"

/*
 * Variables
 */
int fd = -1;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
// int init_process_entry(pid_t pid)
// {

// }

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
 * init and exit proc impl-s
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