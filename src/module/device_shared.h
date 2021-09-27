#pragma once

#include <linux/ioctl.h>

#define UMS_DEV_IOCTL_MAGIC 'R'

typedef struct worker_thread_params {
    unsigned long function;
    unsigned long function_args;
    unsigned long stack_address;
    unsigned long stack_size;
} worker_thread_params_t;

#define UMS_DEV_INIT_UMS_PROCESS        _IO(UMS_DEV_IOCTL_MAGIC, 0)
#define UMS_DEV_EXIT_UMS_PROCESS        _IO(UMS_DEV_IOCTL_MAGIC, 1)
#define UMS_DEV_CREATE_COMPLETION_LIST  _IO(UMS_DEV_IOCTL_MAGIC, 2)
#define UMS_DEV_CREATE_WORKER_THREAD    _IOR(UMS_DEV_IOCTL_MAGIC, 3, worker_thread_params_t *)
