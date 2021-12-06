
# AOSV Final Project Report
_A.Y. 2020/2021_

Author(s): Sultan Umabraev (1954544) 

# Introduction
The goal of the project was to implement **User Mode thread Scheduling** mechanism for the Linux distribution, inspired by the **User-mode scheduling (UMS)** implementation available in the Windows kernel [1]. UMS is a mechanism that allows applications/programs in the user mode to schedule their own threads without involving the kernel/system scheduler. The main goal and advantage is that thread switching in user mode is "*more efficient than thread pools for managing large numbers of short-duration work items...*" [2].

# Design and Implementation
The main components of the UMS mechanism in this implementation are:
 - *ums thread*, the scheduler that determines the next worker thread to be scheduled and executed
 - *worker thread*, the short-duration work tasks of the application
 - *completion list*, the list of worker threads to be associated with scheduler

The execution flow is shown below: 

As shown in the flow, in order to begin utilization of UMS mechanism the application in user space needs to initialize it by `init_ums()`. It makes IOCTL call to invoke `init_ums_process()` which creates necessary structures associated with the application in kernel space.
After that, UMS mechanism is enabled for this application and user can create completion lists and worker threads. This is done with the help of `create_completion_list()` and `create_worker_thread()`. Worker threads are built with *task routine* and required parameters, and completion lists are filled with worker threads by `add_worker_thread()` manually.
The next important element is a *scheduling routine* which is a scheduling function. Once the completion lists are created and filled with worker threads, a *scheduling routine* needs to be created and together with id of the completion list passed to `enter_ums_scheduling_mode()` to invoke `create_ums_thread()` in kernel space which creates corresponding scheduler context. Then `enter_ums_scheduling_mode()` invokes `pthread_create()` [5] to start new thread in the calling process environment. The new thread starts the execution of the `convert_to_ums_thread()` which converts itself into scheduler. This is done `convert_to_ums_thread()` in kernel space which performs context switching.
From that point pthread is converted to scheduler thread and the *scheduling routine* is being executed. Before starting the scheduling of worker threads, scheduler needs to query a list of currently available/ready worker threads to be run by `dequeue_completion_list_items()` which calls corresponding function in kernel space.
Worker thread scheduling and execution is performed by iterating over the list of ready worker threads and invoking `execute_worker_thread()`. The corresponding function in kernel space `switch_to_worker_thread()` performs context switching operation from scheduler to worker thread context. Once worker thread finishes its routine, `worker_thread_yield()` is called with specified yield reason which determines whether worker thread is pausing or finishing. Similarly to worker thread execution, the context switching from worker thread back to scheduler is carried out on worker thread yield in kernel space by `switch_back_to_ums_thread()`.
Finished *scheduling routine* calls `exit_ums_scheduling_mode()` to invoke `convert_from_ums_thread()` on the kernel space to convert scheduler back to pthread. And after that each converted from scheduler thread calls `exit_ums()` to synchronize the execution of schedulers.

# Results
The implementation was built on the following environment:
- Operating System: Ubuntu 20.04.2 LTS
- Kernel: Linux 5.11.0-40-generic
- Architecture: x86-64

The log information examples of the library and module are shown the following pictures:

The measured benchmark sample of the project is the average time needed for a scheduler thread to switch a worker thread which is in range between 150-250 ms. 

The process of executing the worker threads is straight-forward and not optimized completely. Scheduler routine in user space queries(dequeue) a list of available and ready to run worker threads and stores it locally. Thus, when 2 scheduler share a completion list, it is not updated globally. Scheduler updates a local list during the execution attempt of a worker thread, specifically when it sees that the status of the worker thread is finished. Thus, there might be cases when one scheduler finishes the whole list and another scheduler is just trailing behind and updating local list.
Another case to be noted and reported is when one scheduler finishes the list even before the second scheduler has started. 

# References
1) https://gpm.name/teaching/2021-aosv/news/2021/04/09/final-project-track/
2) https://docs.microsoft.com/en-us/windows/win32/procthread/user-mode-scheduling
3) https://www.youtube.com/watch?v=PYlP8MXRCZc
4) https://patents.google.com/patent/US20100083275A1/en
5) https://man7.org/linux/man-pages/man7/pthreads.7.html
6) https://www.mcs.anl.gov/~kazutomo/list/list.h 
