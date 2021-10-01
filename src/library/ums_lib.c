#include "ums_lib.h"

/*
 * Variables
 */
int fd = -1;

cl_list_t cl_list = {
    .list = LIST_HEAD_INIT(cl_list.list),
    .cl_count = 0
};

worker_thread_list_t worker_thread_list = {
    .list = LIST_HEAD_INIT(worker_thread_list.list),
    .worker_thread_count = 0
};

ums_thread_list_t ums_thread_list = {
    .list = LIST_HEAD_INIT(ums_thread_list.list),
    .ums_thread_count = 0
};

/* 
 * Implementations
 */
int init_ums()
{
    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_INIT_UMS_PROCESS);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    return ret;
}

int exit_ums()
{
    synch_pthreads();

    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_EXIT_UMS_PROCESS);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    free_ums_thread();
    free_completion_list();
    free_worker_thread();

    return ret;
}

int create_completion_list()
{
    completion_list_t *completion_list;

    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_CREATE_COMPLETION_LIST);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    completion_list = (completion_list_t *)malloc(sizeof(completion_list_t));
    completion_list->id = ret;
    list_add_tail(&completion_list->list, &cl_list.list);
    cl_list.cl_count++;

    return ret;
}

int create_worker_thread(void (*function)(void *), void *args, unsigned long stack_size)
{
    worker_thread_t *worker_thread;

    worker_thread_params_t *params = (worker_thread_params_t *)malloc(sizeof(worker_thread_params_t));
    params->function = (unsigned long)function;
    params->function_args = (unsigned long)args;
    params->stack_address = (unsigned long)malloc(stack_size) + stack_size;
    params->stack_size = stack_size;

    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_CREATE_WORKER_THREAD, (unsigned long)params);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    worker_thread = (worker_thread_t *)malloc(sizeof(worker_thread_t));
    worker_thread->id = ret;
    worker_thread->params = params;
    list_add_tail(&worker_thread->list, &worker_thread_list.list);
    worker_thread_list.worker_thread_count++;

    return ret;
}

int add_worker_thread(unsigned int completion_list_id, unsigned int worker_thread_id)
{
    completion_list_t *completion_list = get_cl_with_id(completion_list_id);
    worker_thread_t *worker_thread = get_wt_with_id(worker_thread_id);

    add_wt_params_t *params = (add_wt_params_t *)malloc(sizeof(add_wt_params_t));
    params->completion_list_id = completion_list_id;
    params->worker_thread_id = worker_thread_id;

    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_ADD_TO_COMPLETION_LIST, (unsigned long)params);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    completion_list->worker_thread_count++;

    return ret;
}

int *pthread_print_test(unsigned int ums_id)
{
    printf("pthread ums id: %d\n", ums_id);
    return 0;
}

int enter_ums_scheduling_mode(void (*function)(void *), unsigned long completion_list_id)
{
    ums_thread_t *ums_thread;

    ums_thread_params_t *params = (ums_thread_params_t *)malloc(sizeof(ums_thread_params_t));
    params->function = (unsigned long)function;
    params->completion_list_id = completion_list_id;

    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_CREATE_UMS_THREAD, (unsigned long)params);
    if (ret < 0) 
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    ums_thread = (ums_thread_t *)malloc(sizeof(ums_thread_t));
    ums_thread->id = ret;
    ums_thread->params = params;
    list_add_tail(&ums_thread->list, &ums_thread_list.list);
    ums_thread_list.ums_thread_count++;

    ret = pthread_create(&ums_thread->pt, NULL, pthread_print_test, ums_thread->id);

    return ret;
}

/* 
 * Auxiliary functions
 */
int open_dev(void)
{
    fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0)
    {
		perror("Error opening " UMS_DEVICE_PATH);
		return -1;
	}

    return fd;
}

completion_list_t *get_cl_with_id(unsigned int completion_list_id)
{
    if (list_empty(&cl_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty cl list\n");
        return NULL;
    }

    completion_list_t *completion_list = NULL;
    completion_list_t *temp = NULL;
    list_for_each_entry_safe(completion_list, temp, &cl_list.list, list) {
        if (completion_list->id == completion_list_id)
        {
            break;
        }
    }

    return completion_list;
}

worker_thread_t *get_wt_with_id(unsigned int worker_thread_id)
{
    if (list_empty(&worker_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty wt list\n");
        return NULL;
    }

    worker_thread_t *worker_thread = NULL;
    worker_thread_t *temp = NULL;
    list_for_each_entry_safe(worker_thread, temp, &worker_thread_list.list, list) {
        if (worker_thread->id == worker_thread_id)
        {
            break;
        }
    }

    return worker_thread;
}

int free_ums_thread(void)
{
    if (list_empty(&ums_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty umst list\n");
        return -1;
    }

    ums_thread_t *ums_thread = NULL;
    ums_thread_t *temp = NULL;
    list_for_each_entry_safe(ums_thread, temp, &ums_thread_list.list, list) {
        list_del(&ums_thread->list);
        free(ums_thread);
        ums_thread_list.ums_thread_count--;
    }

    return 0;
}

int free_completion_list(void)
{
    if (list_empty(&cl_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty umst list\n");
        return -1;
    }

    completion_list_t *completion_list = NULL;
    completion_list_t *temp = NULL;
    list_for_each_entry_safe(completion_list, temp, &cl_list.list, list) {
        list_del(&completion_list->list);
        free(completion_list);
        cl_list.cl_count--;
    }

    return 0;
}

int free_worker_thread(void)
{
    if (list_empty(&worker_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty umst list\n");
        return -1;
    }

    worker_thread_t *worker_thread = NULL;
    worker_thread_t *temp = NULL;
    list_for_each_entry_safe(worker_thread, temp, &worker_thread_list.list, list) {
        list_del(&worker_thread->list);
        free(worker_thread);
        worker_thread_list.worker_thread_count--;
    }

    return 0;
}

int synch_pthreads(void)
{
    if (list_empty(&ums_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty umst list\n");
        return -1;
    }

    ums_thread_t *ums_thread = NULL;
    ums_thread_t *temp = NULL;
    list_for_each_entry_safe(ums_thread, temp, &ums_thread_list.list, list) {
        pthread_join(ums_thread->pt, NULL);
    }

    return 0;
}

__attribute__((constructor))
void constructor(void)
{
    printf(UMS_LIB_LOG "init() constructor called\n");
}

__attribute__((destructor))
void destructor(void)
{
    printf(UMS_LIB_LOG "exit() destructor called\n");

    close(fd);
}