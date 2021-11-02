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
 * @brief This file contains the implementation of the functions of the /proc part
 *
 * @file proc.c
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

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
static int open_umst_entry(struct inode *inode, struct file *file);
static int show_umst_entry(struct seq_file *sf, void *v);

static int open_wt_entry(struct inode *inode, struct file *file);
static int show_wt_entry(struct seq_file *sf, void *v);

static struct proc_ops umst_entry_fops = {
    .proc_open = open_umst_entry,
    .proc_release = single_release,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
};

static struct proc_ops wt_entry_fops = {
    .proc_open = open_wt_entry,
    .proc_release = single_release,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
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

    process_entry_t *process_entry = NULL;
    process_entry_t *temp_process = NULL;
    list_for_each_entry_safe(process_entry, temp_process, &process_entry_list.list, list) {
        list_del(&process_entry->list);
        kfree(process_entry);
        process_entry_list.process_entry_count--;
    }

    ums_thread_entry_t *ums_thread_entry = NULL;
    ums_thread_entry_t *temp_umst = NULL;
    list_for_each_entry_safe(ums_thread_entry, temp_umst, &ums_thread_entry_list.list, list) {
        list_del(&ums_thread_entry->list);
        kfree(ums_thread_entry);
        ums_thread_entry_list.ums_thread_entry_count--;
    }

    worker_thread_entry_t *worker_thread_entry = NULL;
    worker_thread_entry_t *temp_wt = NULL;
    list_for_each_entry_safe(worker_thread_entry, temp_wt, &worker_thread_entry_list.list, list) {
        list_del(&worker_thread_entry->list);
        kfree(worker_thread_entry);
        worker_thread_entry_list.worker_thread_entry_count--;
    }
}

int create_process_entry(pid_t pid)
{
    printk(KERN_DEBUG UMS_PROC_LOG "--------- Invoking [INIT PROCESS ENTRY]\n");

    process_entry_t *process_entry;

    process_entry = kmalloc(sizeof(process_entry_t), GFP_KERNEL);
    process_entry->pid = pid;
    list_add_tail(&process_entry->list, &process_entry_list.list);
    process_entry_list.process_entry_count++;

    char entry_name[32];
    if (!(snprintf(entry_name, 32, "%lu", (unsigned long)pid))) {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] reading pid");
        return -ERROR_PROC_FAIL;
    }
    process_entry->entry = proc_mkdir(entry_name, ums_entry);
    if(!process_entry->entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] creating /proc/ums/<PID> entry");
        return -ERROR_PROC_FAIL;
    }

    process_entry->schedulers_entry = proc_mkdir("schedulers", process_entry->entry);
    if(!process_entry->schedulers_entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] creating /proc/ums/<PID>/schedulers entry");
        return -ERROR_PROC_FAIL;
    }

    printk(KERN_DEBUG UMS_PROC_LOG "[INIT PROCESS ENTRY] name = %d, process entry count = %d\n", process_entry->pid, process_entry_list.process_entry_count);

    return 0;
}

int create_umst_entry(pid_t pid, unsigned int umst_id)
{
    printk(KERN_DEBUG UMS_PROC_LOG "--------- Invoking [INIT UMST ENTRY]\n");

    process_entry_t *process_entry;
    ums_thread_entry_t *ums_thread_entry;

    process_entry = get_process_entry_with_pid(pid);
    if (process_entry == NULL)
    {
        return -ERROR_PROCESS_ENTRY_NOT_FOUND;
    }

    ums_thread_entry = kmalloc(sizeof(ums_thread_entry_t), GFP_KERNEL);
    ums_thread_entry->id = umst_id;
    list_add_tail(&ums_thread_entry->list, &ums_thread_entry_list.list);
    ums_thread_entry_list.ums_thread_entry_count++;

    char entry_name[32];
    if(!(snprintf(entry_name, 32, "%u", umst_id))) {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] reading umst_id");
        return -ERROR_PROC_FAIL;
    }
    ums_thread_entry->entry = proc_mkdir(entry_name, process_entry->schedulers_entry);
    if(!ums_thread_entry->entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] creating /proc/ums/<PID>/schedulers/<ID> entry");
        return -ERROR_PROC_FAIL;
    }

    ums_thread_entry->workers_entry = proc_mkdir("workers", ums_thread_entry->entry);
    if(!ums_thread_entry->workers_entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] creating /proc/ums/<PID>/schedulers/<ID>/workers entry");
        return -ERROR_PROC_FAIL;
    }

    ums_thread_entry->info_entry = proc_create("info", S_IALLUGO, ums_thread_entry->entry, &umst_entry_fops);
    if(!ums_thread_entry->info_entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] creating /proc/ums/<PID>/schedulers/<ID>/info entry");
        return -ERROR_PROC_FAIL;
    }

    printk(KERN_DEBUG UMS_PROC_LOG "[INIT UMST ENTRY] name = %d, umst entry count = %d\n", ums_thread_entry->id, ums_thread_entry_list.ums_thread_entry_count);

    return 0;
}

int create_wt_entry(unsigned int umst_id, unsigned int wt_id)
{
    printk(KERN_DEBUG UMS_PROC_LOG "--------- Invoking [INIT WT ENTRY]\n");

    ums_thread_entry_t *ums_thread_entry;
    worker_thread_entry_t *worker_thread_entry;

    ums_thread_entry = get_ums_thread_entry_with_pid(umst_id);
    if (ums_thread_entry == NULL)
    {
        return -ERROR_UMST_ENTRY_NOT_FOUND;
    }

    worker_thread_entry = kmalloc(sizeof(worker_thread_entry_t), GFP_KERNEL);
    worker_thread_entry->id = wt_id;
    list_add_tail(&worker_thread_entry->list, &worker_thread_entry_list.list);
    worker_thread_entry_list.worker_thread_entry_count++;

    char entry_name[32];
    if(!(snprintf(entry_name, 32, "%u", wt_id))) {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] reading wt_id");
        return -ERROR_PROC_FAIL;
    }
    worker_thread_entry->entry = proc_create(entry_name, S_IALLUGO, ums_thread_entry->workers_entry, &wt_entry_fops);
    if(!worker_thread_entry->entry)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] creating /proc/ums/<PID>/schedulers/<ID>/workers/<ID> entry");
        return -ERROR_PROC_FAIL;
    }

    printk(KERN_DEBUG UMS_PROC_LOG "[INIT WT ENTRY] name = %d, wt entry count = %d\n", worker_thread_entry->id, ums_thread_entry_list.ums_thread_entry_count);

    return 0;
}

/*
 * Static function impl-s
 */
static int open_umst_entry(struct inode *inode, struct file *file)
{
    int ret = 0;
    unsigned int pid;
    unsigned int umst_id;

    // /proc/ums/<PID>/schedulers/<ID>/info
    if (kstrtoint(file->f_path.dentry->d_parent->d_name.name, 10, &umst_id) != 0)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] open_umst_entry() umst_id error");
        return -ERROR_PROC_FAIL;
    }
    if (kstrtoint(file->f_path.dentry->d_parent->d_parent->d_parent->d_name.name, 10, &pid) != 0)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] open_umst_entry() pid error");
        return -ERROR_PROC_FAIL;
    }

    printk(KERN_DEBUG UMS_PROC_LOG "pid = %lu, umst_id = %lu\n", pid, umst_id);

    process_t *process = get_process_with_pid(pid);
    if (process == NULL)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] process with pid = %d does not exist\n", pid);
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    ums_thread_context_t *ums_thread_context = get_umst_with_id(process, umst_id);
    if (ums_thread_context == NULL)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] umst with id = %d does not exist\n", umst_id);
        return -ERROR_UMS_THREAD_NOT_FOUND;
    }

    ret = single_open(file, show_umst_entry, ums_thread_context);

    return ret;
}

static int open_wt_entry(struct inode *inode, struct file *file)
{
    int ret = 0;
    unsigned int pid;
    unsigned int wt_id;

    // /proc/ums/<PID>/schedulers/<ID>/workers/<ID>
    if (kstrtoint(file->f_path.dentry->d_name.name, 10, &wt_id) != 0)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] open_wt_entry() wt_id error");
        return -ERROR_PROC_FAIL;
    }
    if (kstrtoint(file->f_path.dentry->d_parent->d_parent->d_parent->d_parent->d_name.name, 10, &pid) != 0)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] open_wt_entry() pid error");
        return -ERROR_PROC_FAIL;
    }

    process_t *process = get_process_with_pid(pid);
    if (process == NULL)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] process with pid = %d does not exst\n", pid);
        return -ERROR_PROCESS_NOT_INITIALIZED;
    }
    worker_thread_context_t *worker_thread_context = get_wt_with_id(process, wt_id);
    if (worker_thread_context == NULL)
    {
        printk(KERN_ALERT UMS_PROC_LOG "[ERROR] wt with id = %d does not exst\n", wt_id);
        return -ERROR_WORKER_THREAD_NOT_FOUND;
    }

    ret = single_open(file, show_wt_entry, worker_thread_context);

    return ret;
}

static int show_umst_entry(struct seq_file *sf, void *v)
{
    ums_thread_context_t *ums_thread_context = (ums_thread_context_t *)sf->private;
    seq_printf(sf, "%-25s : %u\n", "ums thread id", ums_thread_context->id);
    seq_printf(sf, "%-25s : %#lx\n", "entry point", ums_thread_context->entry_point);
    seq_printf(sf, "%-25s : %u\n", "completion list id", ums_thread_context->cl_id);
    if (ums_thread_context->state == 0)
    {
        seq_printf(sf, "%-25s : RUNNING\n", "state");
        seq_printf(sf, "%-25s : %u\n", "pthread runner pid", (unsigned int)ums_thread_context->run_by);
        seq_printf(sf, "%-25s : %u\n", "worker thread id", ums_thread_context->wt_id);
    }
    else
    {
        seq_printf(sf, "%-25s : IDLE\n", "state");
    }
    seq_printf(sf, "%-25s : %u\n", "creator pid", (unsigned int)ums_thread_context->created_by);
    seq_printf(sf, "%-25s : %u\n", "switch count", ums_thread_context->switch_count);
    seq_printf(sf, "%-25s : %lu\n", "last switch time (ns)", ums_thread_context->last_switch_time.tv_nsec);

    return 0;
}

static int show_wt_entry(struct seq_file *sf, void *v)
{
    worker_thread_context_t *worker_thread_context = (worker_thread_context_t *)sf->private;
    seq_printf(sf, "%-25s : %u\n", "worker thread id", worker_thread_context->id);
    seq_printf(sf, "%-25s : %#lx\n", "entry point", worker_thread_context->entry_point);
    seq_printf(sf, "%-25s : %u\n", "completion list id", worker_thread_context->cl_id);
    if (worker_thread_context->state == 0)
    {
        seq_printf(sf, "%-25s : BUSY\n", "state");
        seq_printf(sf, "%-25s : %u\n", "ums thread runner id", (unsigned int)worker_thread_context->run_by);
    }
    else if (worker_thread_context->state == 1)
    {
        seq_printf(sf, "%-25s : READY\n", "state");
    }
    else
    {
        seq_printf(sf, "%-25s : FINISHED\n", "state");
    }
    seq_printf(sf, "%-25s : %u\n", "creator pid", (unsigned int)worker_thread_context->created_by);
    seq_printf(sf, "%-25s : %lu\n", "running time (ms)", get_wt_running_time(worker_thread_context));
    seq_printf(sf, "%-25s : %u\n", "switch count", worker_thread_context->switch_count);
    seq_printf(sf, "%-25s : %lu\n", "last switch time (ns)", worker_thread_context->last_switch_time.tv_nsec);

    return 0;
}

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

ums_thread_entry_t *get_ums_thread_entry_with_pid(unsigned int id)
{
    if (list_empty(&ums_thread_entry_list.list))
    {
        printk(KERN_ALERT UMS_LOG "[ERROR] Empty process entry list\n");
        return NULL;
    }

    ums_thread_entry_t *ums_thread_entry = NULL;
    ums_thread_entry_t *temp = NULL;
    list_for_each_entry_safe(ums_thread_entry, temp, &ums_thread_entry_list.list, list) {
        if (ums_thread_entry->id == id)
        {
            break;
        }
    }

    return ums_thread_entry;
}