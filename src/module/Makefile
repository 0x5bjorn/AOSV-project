KERNEL_PATH = /lib/modules/$(shell uname -r)/build
CURRENT_PATH = $(shell pwd)

obj-m += ums_module.o
UMS_module-objs += device.o module.o

all:
	make -C $(KERNEL_PATH) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNEL_PATH) M=$(CURRENT_PATH) clean