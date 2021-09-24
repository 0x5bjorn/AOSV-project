#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>

#include "ums_lib.h"

void *thread1_func(void *data)
{
	int ret = init_ums();
    printf("ioctl(init) returned %d\n", ret);
	ret = create_completion_list();
    printf("ioctl(create cl) returned %d\n", ret);
	ret = exit_ums();
    printf("ioctl(exit) returned %d\n", ret);
}

void *thread2_func(void *data)
{
	int ret = init_ums();
    printf("ioctl(init) returned %d\n", ret);
	ret = create_completion_list();
    printf("ioctl(create cl1) returned %d\n", ret);
	ret = create_completion_list();
    printf("ioctl(create cl2) returned %d\n", ret);
	ret = exit_ums();
    printf("ioctl(exit) returned %d\n", ret);
}

int main(int argc, char **argv)
{
	pthread_t thread1, thread2;

	int ret1 = pthread_create(&thread1, NULL, thread1_func, NULL);
	int ret2 = pthread_create(&thread2, NULL, thread2_func, NULL);

	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL);

	printf("Thread 1 returns: %d\n",ret1);
	printf("Thread 2 returns: %d\n",ret2);

	return 0;
}
