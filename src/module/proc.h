#include <linux/proc_fs.h>

#include "ums.h"

#define UMS_PROC_LOG "UMS proc: "

#define PROC_BUFFER_SIZE 100

/* 
 * Structs
 */


/*
 * init and exit proc functions
 */
int init_proc(void);
void exit_proc(void);
