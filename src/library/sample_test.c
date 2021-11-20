#include <string.h>

#include "ums_lib.h"

void worker1_func()
{
	int a = 88;

	printf("worker thread 1\n");

	for (int i = 0; i<500; ++i)
	{
		int a = 25 * i;
	}

	worker_thread_yield(PAUSE);

	printf("worker thread 1 repeat, local a = %d\n", a);

	worker_thread_yield(FINISH);
}

void worker2_func(void *a)
{
	printf("worker thread 2: a = %d\n", a);

	for (int i = 0; i<300; ++i)
	{
		int a = 52 * i;
	}

	worker_thread_yield(FINISH);
}

void scheduling_func1()
{
	printf("scheduler 1\n");

	int wt_count = get_wt_count_in_current_umst_cl();
	int ready_wt_list[wt_count];
	dequeue_completion_list_items(ready_wt_list);

	while (check_ready_wt_list(ready_wt_list, wt_count))
	{
		int id = get_next_ready_item(ready_wt_list, wt_count);
		execute_worker_thread(ready_wt_list, wt_count, id);
	}

	exit_ums_scheduling_mode();
}

void scheduling_func2()
{
	printf("scheduler 2\n");

	int wt_count = get_wt_count_in_current_umst_cl();
	int ready_wt_list[wt_count];
	dequeue_completion_list_items(ready_wt_list);

	while (check_ready_wt_list(ready_wt_list, wt_count))
	{
		int id = get_next_ready_item(ready_wt_list, wt_count);
		execute_worker_thread(ready_wt_list, wt_count, id);
	}

	exit_ums_scheduling_mode();
}

int main(int argc, char **argv)
{
	int ret = init_ums();

	int cl1 = create_completion_list();

	int cl2 = create_completion_list();

	// for (int i = 0; i<3; ++i)
	// {
	// 	int wt_id = create_worker_thread(worker1_func, NULL, MIN_STACK_SIZE);
	// 	ret = add_worker_thread(cl1, wt_id);
	// }

	ret = add_worker_thread(cl1, create_worker_thread(worker1_func, NULL, MIN_STACK_SIZE));
	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)5, MIN_STACK_SIZE);
		ret = add_worker_thread(cl2, wt_id);
	}
	ret = add_worker_thread(cl1, create_worker_thread(worker2_func, NULL, MIN_STACK_SIZE));
	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)5, MIN_STACK_SIZE);
		ret = add_worker_thread(cl2, wt_id);
	}
	ret = add_worker_thread(cl1, create_worker_thread(worker1_func, NULL, MIN_STACK_SIZE));
	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)5, MIN_STACK_SIZE);
		ret = add_worker_thread(cl2, wt_id);
	}
	ret = add_worker_thread(cl1, create_worker_thread(worker1_func, NULL, MIN_STACK_SIZE));
	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)5, MIN_STACK_SIZE);
		ret = add_worker_thread(cl2, wt_id);
	}
	ret = add_worker_thread(cl1, create_worker_thread(worker1_func, NULL, MIN_STACK_SIZE));
	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)5, MIN_STACK_SIZE);
		ret = add_worker_thread(cl2, wt_id);
	}

	for (int i = 0; i<2; ++i)
	{
		int wt_id = create_worker_thread(worker2_func, (void *)8, MIN_STACK_SIZE);
		ret = add_worker_thread(cl1, wt_id);
	}

	ret = enter_ums_scheduling_mode(scheduling_func1, cl1);

	ret = enter_ums_scheduling_mode(scheduling_func2, cl1);

	return 0;
}
