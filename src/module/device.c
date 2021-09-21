#include "device.h"

static int is_device_open = 0;

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
 * Static functions
 */
static int open_device(struct inode *inode, struct file *file)
{
    // if (is_device_open) return -EBUSY;
    is_device_open++;
    try_module_get(THIS_MODULE);
    return 0;
}

static int close_device(struct inode *inode, struct file *file)
{
    is_device_open++;
    module_put(THIS_MODULE);
    return 0;
}

static long ioctl_device(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk(KERN_DEBUG "UMS device: IOCTL\n");
    return 0;
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