#include <string.h>

#include "ums_lib.h"

void worker1_func()
{
	int a = 88;

	printf("worker thread 1\n");

	worker_thread_yield(PAUSE);

	printf("worker thread 1 repeat, local a = %d\n", a);

	worker_thread_yield(FINISH);
}

void worker2_func(void *a)
{
	printf("worker thread 2: a = %d\n", a);

	worker_thread_yield(FINISH);
}

void scheduling_func1()
{
	printf("scheduler 1\n");

	time_t t;
	srand((unsigned) time(&t));

	int wt_count = get_wt_count_in_current_umst_cl();
	int ready_wt_list[wt_count];
	dequeue_completion_list_items(ready_wt_list);

    for (int i = 2; i < wt_count; ++i)
    {
		// int rand_num = rand() % wt_count;
        printf("ready_wt_list[%d] = %d\n", i, ready_wt_list[i]);
		execute_worker_thread(i);
	}
	execute_worker_thread(0);
	execute_worker_thread(1);

	exit_ums_scheduling_mode();
}

void scheduling_func2()
{
	printf("scheduler 2\n");

	time_t t;
	srand((unsigned) time(&t));

	int wt_count = get_wt_count_in_current_umst_cl();
	int ready_wt_list[wt_count];
	dequeue_completion_list_items(ready_wt_list);

    // for (int i = 0; i < wt_count; ++i)
    // {
	// 	int rand_num = rand() % wt_count;
    //     printf("ready_wt_list[%d] = %d\n", rand_num, ready_wt_list[rand_num]);
	// 	execute_worker_thread(rand_num);
    // }
	execute_worker_thread(0);
	execute_worker_thread(1);

	exit_ums_scheduling_mode();
}

int main(int argc, char **argv)
{
	int ret = init_ums();

	int cl1 = create_completion_list();

	// int cl2 = create_completion_list();

	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker1_func, NULL, 4096);
		ret = add_worker_thread(cl1, wt_id);
	}

	for (int i = 0; i<3; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)5, 4096);
		ret = add_worker_thread(cl1, wt_id);
	}

	ret = enter_ums_scheduling_mode(scheduling_func1, cl1);

	ret = enter_ums_scheduling_mode(scheduling_func2, cl1);

	ret = exit_ums();

	return 0;
}
