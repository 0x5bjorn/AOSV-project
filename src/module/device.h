#pragma once

#include <asm/uaccess.h>
#include <asm-generic/errno-base.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include "ums.h"
#include "device_shared.h"

#define UMS_DEVICE_NAME "umsdevice"

/*
 * init and exit device functions
 */
int init_device(void);
void exit_device(void);