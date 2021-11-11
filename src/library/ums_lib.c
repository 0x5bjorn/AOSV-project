/**
 * Copyright (C) 2021 Sultan Umarbaev <name.sul27@gmail.com>
 *
 * This file is part of UMS implementation (Library).
 *
 * UMS implementation (Library) is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UMS implementation (Library) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UMS implementation (Library).  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @brief This file contains the implementation of all the functions of the library
 *
 * @file ums_lib.c
 * @author Sultan Umarbaev <name.sul27@gmail.com>
 */

#include "ums_lib.h"

/*
 * Variables
 */
int fd = -1;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

/**
 * @brief Initialize/enable UMS in the program/process
 *
 * In order to start utilizing UMS mechanism, we need to enable UMS for the program/process.
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int init_ums(void)
{
    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_INIT_UMS_PROCESS);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    printf(UMS_LIB_LOG "[INIT UMS]\n");

    return ret;
}

/**
 * @brief Exit/disable UMS in the program/process
 *
 * Clean up all created by the library data structures from the memory. 
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int exit_ums(void)
{
    if (list_empty(&ums_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty umst list\n");
        goto end;
    }

    ums_thread_t *ums_thread = NULL;
    ums_thread_t *temp = NULL;
    list_for_each_entry_safe(ums_thread, temp, &ums_thread_list.list, list) {
        int ret = pthread_join(ums_thread->pt, NULL);
        if(ret < 0)
        {
            printf(UMS_LIB_LOG "[ERROR] pthread join error %d\n", errno);
            goto end;
        }
    }

    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_EXIT_UMS_PROCESS);
    if (ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        goto end;
    }

end:
    close_dev();
    free_ums_thread_list();
    free_cl_list();
    free_worker_thread_list();

    printf(UMS_LIB_LOG "[EXIT UMS]\n");

    return ret;
}

/**
 * @brief Create completion list
 *
 * Create a new completion list and return a corresponding id. Add the completion list to @ref cl_list.
 * 
 * @return @c int completion list id
 */
int create_completion_list(void)
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

    printf(UMS_LIB_LOG "[CREATE CL] completion list id = %d\n", completion_list->id);

    return ret;
}

/**
 * @brief Create worker thread
 *
 * Create a new worker thread and return a corresponding id.

 * @param function the address of the starting function of the worker thread
 * @param args the address of arguments allocated by the user passed to 
 * the function (first parameter)
 * @param stack_size the stack size that is used for calculating the stack address 
 * after memory allocation with malloc function
 * @return @c int worker thread id
 */
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
        free(params);
        return -1;
    }

    worker_thread = (worker_thread_t *)malloc(sizeof(worker_thread_t));
    worker_thread->id = ret;
    worker_thread->params = params;
    list_add_tail(&worker_thread->list, &worker_thread_list.list);
    worker_thread_list.worker_thread_count++;

    printf(UMS_LIB_LOG "[CREATE WT] worker thread id = %d\n", worker_thread->id);

    return ret;
}

/**
 * @brief Add worker thread to completion list
 *
 * @param completion_list_id the id of completion list to which worker thread is added
 * @param worker_thread_id the id of worker thread that is added
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
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
        free(params);
        return -1;
    }

    completion_list->worker_thread_count++;

    printf(UMS_LIB_LOG "[ADD WT TO CL] worker thread id = %d, completion list id = %d\n", worker_thread->id, completion_list->id);

    free(params);

    return ret;
}

/**
 * @brief Enter UMS scheduling mode
 *
 * Create ums thread(scheduler) and pthread which will be converted to ums thread created earlier.
 * 
 * @param function an entry point function for the ums thread(scheduler), scheduling function
 * @param completion_list_id the id of completion list with worker threads associated with 
 * ums thread(scheduler)
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
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
        free(params);
        return -1;
    }

    ums_thread = (ums_thread_t *)malloc(sizeof(ums_thread_t));
    ums_thread->id = ret;
    ums_thread->params = params;
    list_add_tail(&ums_thread->list, &ums_thread_list.list);
    ums_thread_list.ums_thread_count++;

    printf(UMS_LIB_LOG "[ENTER UMS SCH MODE] ums scheduler id = %d\n", ums_thread->id);

    ret = pthread_create(&ums_thread->pt, NULL, convert_to_ums_thread, (void *)ums_thread->id);
    if (ret < 0) 
    {
        printf(UMS_LIB_LOG "[ERROR] pthread create error %d\n", errno);
        free(params);
        return -1;
    }

    return ret;
}

/**
 * @brief Convert pthread into ums thread(scheduler)
 *
 * Convert current thread into ums thread(scheduler). This function is passed to @ref pthread_create(),
 * therefore the created pthread is converted into ums thread(scheduler).
 * 
 * @param @c int the id of ums thread(scheduler) into which to convert
 */
void *convert_to_ums_thread(void *ums_thread_id)
{
    unsigned int *ums_id = (unsigned int *)ums_thread_id;

    int ret = open_dev();
    if (ret < 0) 
    {
        printf(UMS_LIB_LOG "[ERROR] opening during convert %d\n", errno);
        pthread_exit(NULL);
    }

    printf(UMS_LIB_LOG "[CONVERT TO UMS] ums scheduler id = %d\n", ums_thread_id);

    ret = ioctl(ret, UMS_DEV_CONVERT_TO_UMS_THREAD, (unsigned long)ums_id);
    if (ret < 0) 
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

/**
 * @brief Exit UMS scheduling mode
 *
 * Convert from ums thread(scheduler) back to pthread.
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int exit_ums_scheduling_mode(void)
{
    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_CONVERT_FROM_UMS_THREAD);
    if(ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }
    
    printf(UMS_LIB_LOG "[CONVERT FROM UMS]\n");

    return ret;
}

/**
 * @brief Dequeue completion list 
 *
 * Obtain a set of currently available worker threads to be run. 
 * 
 * @param ready_wt_list the pointer to an allocated array of integers which will be filled with
 * ready to run worker thread ids
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int dequeue_completion_list_items(int *ready_wt_list)
{
    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_DEQUEUE_CL_ITEMS, (unsigned long)ready_wt_list);
    if(ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    printf(UMS_LIB_LOG "[DEQUEUE CL]\n");

    return ret;
}

/**
 * @brief Execute specific worker thread
 *
 * Execute worker thread given the id. The return result from ioctl call defines if requested 
 * worker thread is currently busy and handled by another ums thread(scheduler) or worker thread 
 * was already finished before. If it is busy then scheduler will try to execute next available thread
 * from @ref ready_wt_list. However, if it was already finished before @ref ready_wt_list will be updated.
 * 
 * @param ready_wt_list the pointer to an array of ready to run worker thread ids
 * @param size the size of the array
 * @param worker_thread_id the id of the worker thread to be executed
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int execute_worker_thread(int *ready_wt_list, int size, unsigned int worker_thread_id)
{
    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_SWITCH_TO_WORKER_THREAD, (unsigned long)worker_thread_id);
    if(ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }
    
    if (ret == 2)
    {
        printf(UMS_LIB_LOG "[WARNING] worker thread with id = %d is BUSY\n", worker_thread_id);
        
        int i = 0;
        while (i < size && (ready_wt_list[i] == worker_thread_id || ready_wt_list[i] == -1)) ++i;
        
        if (i == size)
        {
            printf(UMS_LIB_LOG "[WARNING] no available READY worker threads remain\n");
            return ret;
        }
        
        worker_thread_id = ready_wt_list[i];
        printf(UMS_LIB_LOG "[WARNING] attempting to execute next READY worker thread, id = %d\n", worker_thread_id);
        ret = ioctl(fd, UMS_DEV_SWITCH_TO_WORKER_THREAD, (unsigned long)(worker_thread_id));
    }

    if (ret == 1)
    {
        printf(UMS_LIB_LOG "[ERROR] worker thread with id = %d is FINISHED\n", worker_thread_id);
        int i = 0;
        while (i < size && ready_wt_list[i] != worker_thread_id) ++i;
        ready_wt_list[i] = -1;
    }

    printf(UMS_LIB_LOG "[EXECUTE WT] wt id = %d\n", worker_thread_id);

    return ret;
}

/**
 * @brief Pause or finish worker thread 
 *
 * Pause or finish worker thread deppending on the passed reason.
 * 
 * @param yield_reason reason which defines if worker thread should be paused or finished, @ref yield_reason_t
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int worker_thread_yield(yield_reason_t yield_reason)
{
    fd = open_dev();

    int ret = ioctl(fd, UMS_DEV_SWITCH_BACK_TO_UMS_THREAD, (unsigned long)yield_reason);
    if(ret < 0)
    {
        printf(UMS_LIB_LOG "[ERROR] ioctl errno %d\n", errno);
        return -1;
    }

    printf(UMS_LIB_LOG "[WT YIELD] yield reason = %d\n", yield_reason);

    return ret;
}

/**
 * @brief Get next item from the list of ready worker threads
 *
 * @param ready_wt_list the pointer to an array of ready to run worker thread ids
 * @param size the size of the array
 * @return @c int worker thread id
 */
int get_next_ready_item(int *ready_wt_list, int size)
{
    int i = -1;
    while (++i < size && ready_wt_list[i] == -1);
    return ready_wt_list[i];
}

/**
 * @brief Check if the list of ready worker threads is empty
 *
 * @param ready_wt_list the pointer to an array of ready to run worker thread ids
 * @param size the size of the array
 * @return @c int 0 if false, otherwise true
 */
int check_ready_wt_list(int *ready_wt_list, int size)
{
    while (--size > -1 && ready_wt_list[size] == -1);
    return size != -1;
}

/* 
 * Auxiliary functions
 */

/**
 * @brief Open /dev/umsdevice device
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int open_dev(void)
{
    // pthread_mutex_lock(&mutex);
    if(fd < 0)
    {
        fd = open(UMS_DEVICE_PATH, O_RDONLY);
        if(fd < 0)
        {
            perror("Error opening " UMS_DEVICE_PATH);
            return -1;
        }
    }
    // pthread_mutex_unlock(&mutex);

    return fd;
}

/**
 * @brief Close /dev/umsdevice device
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int close_dev(void)
{
    // pthread_mutex_lock(&mutex);
    fd = close(fd);
    if(fd < 0)
    {
        perror("Error closing " UMS_DEVICE_PATH);
        return -1;
    }
    // pthread_mutex_unlock(&mutex);

    return fd;
}

/**
 * @brief Get the number of worker threads in completion list of current ums thread(scheduler)
 * 
 * @return @c int number of worker threads
 */
int get_wt_count_in_current_umst_cl(void)
{
    pthread_t current_pt = pthread_self();
    ums_thread_t *ums_thread = get_umst_run_by_pthread(current_pt);
    completion_list_t *completion_list = get_cl_with_id(ums_thread->params->completion_list_id);
    
    return completion_list->worker_thread_count;
}

/**
 * @brief Get completion list from @ref cl_list_t
 * 
 * @param completion_list_id id of the completion list requested to retrieve
 * @return @c completion_list_t the pointer to completion list with specific id
 */
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

/**
 * @brief Get worker thread from @ref worker_thread_list_t
 * 
 * @param worker_thread_id id of the worker thread requested to retrieve
 * @return @c worker_thread_t the pointer to worker thread with specific id
 */
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

/**
 * @brief Get ums thread(scheduler) from @ref ums_thread_list_t
 * 
 * @param ums_thread_id id of the ums thread(scheduler) requested to retrieve
 * @return @c ums_thread_t the pointer to ums thread(scheduler) with specific id
 */
ums_thread_t *get_umst_run_by_pthread(pthread_t current_pt)
{
    if (list_empty(&ums_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty wt list\n");
        return NULL;
    }

    ums_thread_t *ums_thread = NULL;
    ums_thread_t *temp = NULL;
    list_for_each_entry_safe(ums_thread, temp, &ums_thread_list.list, list) {
        if (pthread_equal(ums_thread->pt, current_pt))
        {
            break;
        }
    }

    return ums_thread;
}

/**
 * @brief Clean the list of ums threads(schedulers)
 * 
 * Delete and free each item in the list of ums threads(schedulers)
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_ums_thread_list(void)
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
        free(ums_thread->params);
        free(ums_thread);
        ums_thread_list.ums_thread_count--;
    }

    return 0;
}

/**
 * @brief Clean the list of completion lists
 * 
 * Delete and free each item in the list of completion lists
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_cl_list(void)
{
    if (list_empty(&cl_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty cl list\n");
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

/**
 * @brief Clean the list of worker threads
 * 
 * Delete and free each item in the list of worker threads
 * 
 * @return @c int exit code 0 for success, otherwise a corresponding error code
 */
int free_worker_thread_list(void)
{
    if (list_empty(&worker_thread_list.list))
    {
        printf(UMS_LIB_LOG "[ERROR] Empty wt list\n");
        return -1;
    }

    worker_thread_t *worker_thread = NULL;
    worker_thread_t *temp = NULL;
    list_for_each_entry_safe(worker_thread, temp, &worker_thread_list.list, list) {
        list_del(&worker_thread->list);
        free((void *)(worker_thread->params->stack_address - worker_thread->params->stack_size));
        free(worker_thread->params);
        free(worker_thread);
        worker_thread_list.worker_thread_count--;
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
    exit_ums();
    printf(UMS_LIB_LOG "exit() destructor called\n");
}