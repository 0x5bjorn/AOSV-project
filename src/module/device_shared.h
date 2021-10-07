#pragma once

#include <linux/ioctl.h>

#define UMS_DEV_IOCTL_MAGIC 'R'

typedef struct worker_thread_params {
    unsigned long function;
    unsigned long function_args;
    unsigned long stack_address;
    unsigned long stack_size;
} worker_thread_params_t;

typedef struct add_wt_params {
    unsigned int completion_list_id;
    unsigned int worker_thread_id;
} add_wt_params_t;

typedef struct ums_thread_params {
    unsigned long function;
    unsigned int completion_list_id;
} ums_thread_params_t;

#define UMS_DEV_INIT_UMS_PROCESS        _IO(UMS_DEV_IOCTL_MAGIC, 0)
#define UMS_DEV_EXIT_UMS_PROCESS        _IO(UMS_DEV_IOCTL_MAGIC, 1)
#define UMS_DEV_CREATE_COMPLETION_LIST  _IO(UMS_DEV_IOCTL_MAGIC, 2)
#define UMS_DEV_CREATE_WORKER_THREAD    _IOR(UMS_DEV_IOCTL_MAGIC, 3, worker_thread_params_t *)
#define UMS_DEV_ADD_TO_COMPLETION_LIST  _IOR(UMS_DEV_IOCTL_MAGIC, 4, add_wt_params_t *)
#define UMS_DEV_CREATE_UMS_THREAD       _IOR(UMS_DEV_IOCTL_MAGIC, 5, ums_thread_params_t *)
#define UMS_DEV_CONVERT_TO_UMS_THREAD   _IOR(UMS_DEV_IOCTL_MAGIC, 6, int)
#define UMS_DEV_CONVERT_FROM_UMS_THREAD _IO(UMS_DEV_IOCTL_MAGIC, 7)
#define UMS_DEV_DEQUEUE_CL_ITEMS        _IOWR(UMS_DEV_IOCTL_MAGIC, 8, int *)
