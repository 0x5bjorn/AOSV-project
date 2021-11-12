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
 * @brief This file contains the implementation of the functions of the char device part
 *
 * @file device.c
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

#include "device.h"

spinlock_t spinlock;
DEFINE_SPINLOCK(spinlock);
unsigned long sl_irq_flags;

/*
 * Static functions
 */
static int open_device(struct inode *, struct file *);
static int close_device(struct inode *, struct file *);
static long ioctl_device(struct file *file, unsigned int cmd, unsigned long arg);

/*
 * Variables
 */
// File operations for the module
static struct file_operations fops = {
    .open = open_device,
    .release = close_device,
	.unlocked_ioctl = ioctl_device
};

static struct miscdevice mdev;

/*
 * Static function impl-s
 */
static int open_device(struct inode *inode, struct file *file)
{
    try_module_get(THIS_MODULE);
    return 0;
}

static int close_device(struct inode *inode, struct file *file)
{
    module_put(THIS_MODULE);
    return 0;
}

/**
 * @brief The main function handler of the ioctl calls of the module.
 *
 * @param file the pointer to file strcture
 * @param cmd the command number
 * @param arg the pointer to arguments passed from user space
 * @return @c int command return result
 */
static long ioctl_device(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;

    spin_lock_irqsave(&spinlock, sl_irq_flags);

    switch (cmd)
    {
        case UMS_DEV_INIT_UMS_PROCESS:
            ret = init_ums_process();
            break;
        case UMS_DEV_EXIT_UMS_PROCESS:
            ret = exit_ums_process();
            break;
        case UMS_DEV_CREATE_COMPLETION_LIST:
            ret = create_completion_list();
            break;
        case UMS_DEV_CREATE_WORKER_THREAD:
            ret = create_worker_thread((worker_thread_params_t *) arg);
            break;
        case UMS_DEV_ADD_TO_COMPLETION_LIST:
            ret = add_to_completion_list((add_wt_params_t *) arg);
            break;
        case UMS_DEV_CREATE_UMS_THREAD:
            ret = create_ums_thread((ums_thread_params_t *) arg);
            break;
        case UMS_DEV_CONVERT_TO_UMS_THREAD:
            ret = convert_to_ums_thread((unsigned int) arg);
            break;
        case UMS_DEV_CONVERT_FROM_UMS_THREAD:
            ret = convert_from_ums_thread();
            break;
        case UMS_DEV_DEQUEUE_CL_ITEMS:
            ret = dequeue_completion_list_items((int *) arg);
            break;
        case UMS_DEV_SWITCH_TO_WORKER_THREAD:
            ret = switch_to_worker_thread((unsigned int) arg);
            break;
        case UMS_DEV_SWITCH_BACK_TO_UMS_THREAD:
            ret = switch_back_to_ums_thread((yield_reason_t) arg);
            break;
        default:
            return -1;
            break;
    }

    spin_unlock_irqrestore(&spinlock, sl_irq_flags);

    return ret;
}

/*
 * init and exit device impl-s
 */

/**
 * @brief Init/register the UMS device
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int init_device(void)
{
    mdev.minor = 0;
    mdev.name = UMS_DEVICE_NAME;
    mdev.mode = S_IALLUGO;
    mdev.fops = &fops;

    printk(KERN_DEBUG UMS_DEVICE_LOG "init\n");
    int ret = misc_register(&mdev);

    if (ret < 0)
    {
        printk(KERN_ALERT UMS_DEVICE_LOG "Registering device failed\n");
        return ret;
    }
    printk(KERN_DEBUG UMS_DEVICE_LOG "Device registered successfully\n");

    return ret;
}

/**
 * @brief Exit/deregister the UMS device
 *
 */
void exit_device(void)
{
    misc_deregister(&mdev);
    printk(KERN_DEBUG UMS_DEVICE_LOG "Device deregistered\n");   
}