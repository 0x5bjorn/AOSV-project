#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>

#include "../module/device_shared.h"

#define UMS_DEVICE_PATH "/dev/umsdevice"

/* 
 * Functions
 */
int init_ums();
int exit_ums();
int create_completion_list();