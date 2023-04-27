obj-m := my_driver.o

SRC_DIR = $(shell pwd)
KERN_DIR = /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS=/root/device_mode/my_bus/Module.symvers

modules:
	make -C ${KERN_DIR} M=${SRC_DIR} modules

modules_install:
	make -C ${KERN_DIR} M=${SRC_DIR} modules_install

clean:
	make -C ${KERN_DIR} M=${shell pwd} modules clean
