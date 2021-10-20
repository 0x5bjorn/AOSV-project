#include <linux/proc_fs.h>

#define UMS_PROC_LOG "UMS proc: "

#define PROC_BUFFER_SIZE 100

/* 
 * Structs
 */
typedef struct process_entry_list {
    struct list_head list;
    unsigned int process_entry_count;
} process_entry_list_t;

typedef struct ums_thread_entry_list {
	struct list_head list;
	unsigned int ums_thread_entry_count;
} ums_thread_entry_list_t;

typedef struct worker_thread_entry_list {
	struct list_head list;
	unsigned int worker_thread_entry_count;
} worker_thread_entry_list_t;

typedef struct process_entry {
    pid_t pid;
	struct list_head list;
	struct proc_dir_entry *process_entry;
    struct proc_dir_entry *schedulers_entry;
} process_entry_t;

typedef struct ums_thread_entry {
	unsigned int id;
	struct list_head list;
	struct proc_dir_entry *umst_entry;
    struct proc_dir_entry *workers_entry;
    struct proc_dir_entry *info_entry;
} ums_thread_entry_t;

typedef struct worker_thread_entry {
	unsigned int id;
	struct list_head list;
	struct proc_dir_entry *wt_entry;
    struct proc_dir_entry *info_entry;
} worker_thread_entry_t;

/*
 * Functions
 */
// int init_process_entry(pid_t pid);


/*
 * init and exit proc functions
 */
int init_proc(void);
void exit_proc(void);
