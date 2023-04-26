#!/bin/sh
rmmod mydrv.ko
make clean && make && make modules_install
insmod mydrv.ko
