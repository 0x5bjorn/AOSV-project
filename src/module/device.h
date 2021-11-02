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
 * @brief This file is a header of the char device part of the module
 *
 * This file contains the macros and function declarations of the char device part
 * 
 * @file device.h
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

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
