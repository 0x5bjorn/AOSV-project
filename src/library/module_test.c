#include <string.h>

#include "ums_lib.h"

int main(int argc, char **argv)
{
	int ret = init_ums();
    printf("ioctl(init) returned %d\n", ret);

	int cl1 = create_completion_list();
    printf("ioctl(create cl) returned %d\n", cl1);
	int cl2 = create_completion_list();
    printf("ioctl(create cl) returned %d\n", cl2);

	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(NULL, NULL, 1024);
    	printf("ioctl(create wt) returned %d\n", wt_id);
		ret = add_worker_thread(cl1, wt_id);
    	printf("ioctl(add wt) returned %d\n", ret);
	}

	for (int i = 0; i<3; ++i)
	{
		int wt_id = create_worker_thread(NULL, NULL, 512);
    	printf("ioctl(create wt) returned %d\n", wt_id);
		ret = add_worker_thread(cl2, wt_id);
    	printf("ioctl(add wt) returned %d\n", ret);
	}

	ret = enter_ums_scheduling_mode(NULL, cl1);
    printf("ioctl(create umt) returned %d\n", ret);

	ret = enter_ums_scheduling_mode(NULL, cl2);
    printf("ioctl(create umt) returned %d\n", ret);

	ret = exit_ums();
    printf("ioctl(exit) returned %d\n", ret);

	return 0;
}
