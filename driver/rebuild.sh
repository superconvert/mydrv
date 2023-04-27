#!/bin/sh
rmmod mydrv.ko
make clean && make && make modules_install
insmod mydrv.ko mode=1 port=1,2,3,4 name="mydrv"
