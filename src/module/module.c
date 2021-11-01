/**
 * Copyright (C) 2021 Sultan Umarbaev <name.sul27@gmail.com>
 *
 * This file is part of UMS implementation (Kernel Module).
 *
 * UMS implementation (Kernel Module) is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UMS implementation (Kernel Module) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UMS implementation (Kernel Module).  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "module.h"

MODULE_AUTHOR("Sultan Umarbaev <umarbaev.1954544@studenti.uniroma1.it>");
MODULE_DESCRIPTION("UMS module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

static int __init ums_module_init(void)
{
	int ret = init_device();
	ret = init_proc();
	printk(KERN_INFO "UMS Module: Module loaded\n");
	return ret;
}

static void __exit ums_module_exit(void)
{
	exit_device();
	exit_proc();
	printk(KERN_INFO "UMS Module: Module unloaded\n");
}

module_init(ums_module_init);
module_exit(ums_module_exit);