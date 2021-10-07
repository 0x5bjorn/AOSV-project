#include <string.h>

#include "ums_lib.h"

void scheduling_func1()
{
	printf("scheduler 1\n");

	int wt_count = get_wt_count_in_current_umst_cl();
	int ready_wt_list[wt_count];
	dequeue_completion_list_items(ready_wt_list);

    for (int i = 0; i < wt_count; ++i)
    {
        printf("ready_wt_list[%d] = %d\n", i, ready_wt_list[i]);
    }

	exit_ums_scheduling_mode();
}

void scheduling_func2()
{
	printf("scheduler 2\n");

	int wt_count = get_wt_count_in_current_umst_cl();
	int ready_wt_list[wt_count];
	dequeue_completion_list_items(ready_wt_list);

    for (int i = 0; i < wt_count; ++i)
    {
        printf("ready_wt_list[%d] = %d\n", i, ready_wt_list[i]);
    }
	
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

	ret = enter_ums_scheduling_mode(scheduling_func1, cl1);

	ret = enter_ums_scheduling_mode(scheduling_func2, cl2);

	ret = exit_ums();

	return 0;
}
