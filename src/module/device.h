#pragma once

#include <asm/uaccess.h>
#include <asm-generic/errno-base.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/semaphore.h>

#include "ums.h"
#include "device_shared.h"

#define UMS_DEVICE_NAME "umsdevice"
#define UMS_DEVICE_LOG "UMS device: "

/*
 * init and exit device functions
 */
int init_device(void);
void exit_device(void);
