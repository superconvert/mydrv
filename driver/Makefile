obj-m := mydrv.o

SRC_DIR := $(shell pwd)
KERN_DIR := /lib/modules/$(shell uname -r)/build
CCFLAGS-Y := -std=gnu99 -Wno-declaration-after-statement

modules:
	make -C ${KERN_DIR} M=${SRC_DIR} modules

modules_install:
	make -C ${KERN_DIR} M=${SRC_DIR} modules_install

clean:
	make -C ${KERN_DIR} M=${shell pwd} modules clean
