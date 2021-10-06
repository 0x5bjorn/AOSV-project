#include <string.h>

#include "ums_lib.h"

void thread1_func()
{
	for (int i = 0; i < 6; i++)
	{
		int a = 75 * i;
	}
	printf("thread 1\n");
	exit_ums_scheduling_mode();
}

void thread2_func()
{
	for (int i = 0; i < 4; i++)
	{
		int a = 126 * i;
	}
	printf("thread 2\n");
	exit_ums_scheduling_mode();
}

int main(int argc, char **argv)
{
	int ret = init_ums();

	int cl1 = create_completion_list();

	int cl2 = create_completion_list();

	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(NULL, NULL, 1024);
		ret = add_worker_thread(cl1, wt_id);
	}

	for (int i = 0; i<3; ++i)
	{
		int wt_id = create_worker_thread(NULL, NULL, 512);
		ret = add_worker_thread(cl2, wt_id);
	}

	ret = enter_ums_scheduling_mode(thread1_func, cl1);

	ret = enter_ums_scheduling_mode(thread2_func, cl2);

	ret = exit_ums();

	return 0;
}
