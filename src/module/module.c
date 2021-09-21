#include "module.h"

MODULE_AUTHOR("Sultan Umarbaev <umarbaev.1954544@studenti.uniroma1.it>");
MODULE_DESCRIPTION("UMS module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

static int __init ums_module_init(void)
{
	int ret = init_device();
	printk(KERN_INFO "UMS Module: Module loaded\n");
	return ret;
}

static void __exit ums_module_exit(void)
{
	exit_device();
	printk(KERN_INFO "UMS Module: Module unloaded\n");
}

module_init(ums_module_init);
module_exit(ums_module_exit);