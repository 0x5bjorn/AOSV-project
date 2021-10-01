#include "device.h"

// static int is_device_open = 0;

/*
 * Static functions
 */
static int open_device(struct inode *, struct file *);
static int close_device(struct inode *, struct file *);
static long ioctl_device(struct file *file, unsigned int cmd, unsigned long arg);

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
    // if (is_device_open) return -EBUSY;
    // is_device_open++;
    try_module_get(THIS_MODULE);
    return 0;
}

static int close_device(struct inode *inode, struct file *file)
{
    // is_device_open++;
    module_put(THIS_MODULE);
    return 0;
}

static long ioctl_device(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;

    switch (cmd) {
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
        default:
            return -1;
            break;
    }

    return ret;
}

/*
 * init and exit device impl-s
 */
int init_device(void)
{
    mdev.minor = 0;
    mdev.name = UMS_DEVICE_NAME;
    mdev.mode = S_IALLUGO;
    mdev.fops = &fops;

    printk(KERN_DEBUG "UMS device: init\n");
    int ret = misc_register(&mdev);

    if (ret < 0)
    {
        printk(KERN_ALERT "UMS device: Registering device failed\n");
        return ret;
    }
    printk(KERN_DEBUG "UMS device: Device registered successfully\n");

    return 0;
}

void exit_device(void)
{
    misc_deregister(&mdev);
    printk(KERN_DEBUG "UMS device: Device deregistered\n");   
}