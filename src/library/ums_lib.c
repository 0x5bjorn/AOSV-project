#include "ums_lib.h"

/* 
 * Implementations
 */
int init_ums()
{
    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		exit(EXIT_FAILURE);
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
		exit(EXIT_FAILURE);
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
    int fd = open(UMS_DEVICE_PATH, O_RDONLY);
	if(fd < 0) {
		perror("Error opening " UMS_DEVICE_PATH);
		exit(EXIT_FAILURE);
	}

    int ret = ioctl(fd, UMS_DEV_CREATE_COMPLETION_LIST);
    if (ret < 0) {
        printf( "ums_lib: ioctl error, errno %d\n", errno);
        return -1;
    }

    close(fd);

    return ret;
}