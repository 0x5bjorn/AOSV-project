#include "ums_lib.h"

/* 
 * Implementations
 */
int init_ums()
{
    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		return -1;
	}

    int ret = ioctl(fd, UMS_DEV_INIT_UMS_PROCESS);
    if (ret < 0) {
        printf( "ums_lib: ioctl error, errno %d\n", errno);
        return -1;
    }

    close(fd);

    return ret;
}

int exit_ums()
{
    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		return -1;
	}

    int ret = ioctl(fd, UMS_DEV_EXIT_UMS_PROCESS);
    if (ret < 0) {
        printf( "ums_lib: ioctl error, errno %d\n", errno);
        return -1;
    }

    close(fd);

    return ret;
}

int create_completion_list()
{
    //lib data structure for completion list

    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		return -1;
	}

    int ret = ioctl(fd, UMS_DEV_CREATE_COMPLETION_LIST);
    if (ret < 0) {
        printf( "ums_lib: ioctl error, errno %d\n", errno);
        return -1;
    }

    close(fd);

    return ret;
}

int create_worker_thread(void (*function)(void *), void *args, unsigned long stack_size)
{
    //lib data structure for worker threads

    worker_thread_params_t *params = (worker_thread_params_t *)malloc(sizeof(worker_thread_params_t));
    params->function = (unsigned long)function;
    params->function_args = (unsigned long)args;
    params->stack_address = (unsigned long)malloc(stack_size) + stack_size;
    params->stack_size = stack_size;

    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		return -1;
	}

    int ret = ioctl(fd, UMS_DEV_CREATE_WORKER_THREAD, (unsigned long)params);
    if (ret < 0) {
        printf( "ums_lib: ioctl error, errno %d\n", errno);
        return -1;
    }

    close(fd);

    return ret;
}

int add_worker_thread(unsigned int completion_list_id, unsigned int worker_thread_id)
{
    //lib adding wt data structure to cl of wt

    add_wt_params_t *params = (add_wt_params_t *)malloc(sizeof(add_wt_params_t));
    params->completion_list_id = completion_list_id;
    params->worker_thread_id = worker_thread_id;

    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		return -1;
	}

    int ret = ioctl(fd, UMS_DEV_ADD_TO_COMPLETION_LIST, (unsigned long)params);
    if (ret < 0) {
        printf( "ums_lib: ioctl error, errno %d\n", errno);
        return -1;
    }

    close(fd);

    return ret;
}