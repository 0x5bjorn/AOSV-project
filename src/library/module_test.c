#include <string.h>

#include "ums_lib.h"

void *thread1_func(void *data)
{
	int ret = 0;

	int cl_id = create_completion_list();
    printf("ioctl(create cl) returned %d\n", cl_id);
	
	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(NULL, NULL, 1024);
    	printf("ioctl(create wt) returned %d\n", wt_id);
		ret = add_worker_thread(cl_id, wt_id);
    	printf("ioctl(add wt) returned %d\n", ret);
	}

	ret = create_ums_thread(NULL, cl_id);
    printf("ioctl(create umt) returned %d\n", ret);
}

void *thread2_func(void *data)
{
	int ret = 0;

	int cl_id = create_completion_list();
    printf("ioctl(create cl) returned %d\n", cl_id);

	for (int i = 0; i<3; ++i)
	{
		int wt_id = create_worker_thread(NULL, NULL, 512);
    	printf("ioctl(create wt) returned %d\n", wt_id);
		ret = add_worker_thread(cl_id, wt_id);
    	printf("ioctl(add wt) returned %d\n", ret);
	}

	ret = create_ums_thread(NULL, cl_id);
    printf("ioctl(create umt) returned %d\n", ret);
}

int main(int argc, char **argv)
{
	int ret = init_ums();
    printf("ioctl(init) returned %d\n", ret);

	pthread_t thread1, thread2;

	int ret1 = pthread_create(&thread1, NULL, thread1_func, NULL);
	int ret2 = pthread_create(&thread2, NULL, thread2_func, NULL);

	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL);

	printf("Thread 1 returns: %d\n",ret1);
	printf("Thread 2 returns: %d\n",ret2);

	ret = exit_ums();
    printf("ioctl(exit) returned %d\n", ret);

	return 0;
}
