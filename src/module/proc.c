#include "proc.h"

static ssize_t read_proc(struct file *file, char __user *ubuf, size_t count, loff_t *offset);
static ssize_t write_proc(struct file *file, const char __user *ubuf, size_t count, loff_t *offset);

static struct proc_ops pops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

static struct proc_dir_entry *ums_entry;

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