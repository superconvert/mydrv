# 介绍 
  1. 演示了一个驱动对应多个设备，以及对具体设备的存取
  2. 演示了应用与驱动，mmap 的映射方式的实现与访问
  3. 演示了应用层通过 select, poll, epoll 驱动层的实现, 执行 read 和 write 程序
  4. netlink 的单播，多播，命名空间的使用方式, 执行 unicast 和 multicast, namespace 程序

# 说明
  如果您感觉本项目对您有帮助，麻烦 star 一下本项目或分享给更多的人知道，如果您想了解更多的系统知识，请您移步我的另外一个项目  
  ```
  https://github.com/superconvert/smart-os  
  ```
  一步一步教您打造一个带桌面环境的 Linux 操作系统！！！
  

# 小技巧
 1. 驱动参数可以通过下面的方式进行设置:  
    insmod mydrv.ko mode=1 port=1,2,3,4 name="mydrv"  

 2. 查看驱动日志打印:  
    dmesg -w  

 3. 查看驱动信息:  
    modinfo mydrv.ko  

 4. 常用的驱动命令:  
    insmod, lsmod, rmmod, mknod, depmod, modprobe  
    mknod /dev/xxx c [主设备号] [次设备号]  

 5. 其它相关点:  
    查看设备信息 cat /proc/devices  
    查看设备树   ls /proc/device-tree  
    查看设备文件 ls /dev  

# driver [ 配合 app 目录使用 ]
  1. 删除驱动 rmmod mydrv.ko
  2. 一般内核都会对模块进行签名检查，没好的办法，关闭内核源码的对模块签名，重新编译内核，用新内核启动 CONFIG_MODULE_SIG=n

# app [ 配合 driver 目录使用 ]

  1. mmap 是演示应用层与驱动层之间的使用 mmap

  2. read 和 write 演示了利用 select, poll, epoll 模型从内核读取东西

  3. netlink_unicast 需要驱动层打开 NETLINK_UNICAST 宏定义

     Compile kernel module and user space program:
     make

     Load kernel module:
     insmod ./netlink_test.ko

     Also check kernel log `dmesg` for module debug output:
     ./nl_recv "Hello you!"
     Hello you!

     Unload kernel module:
     rmmod netlink_test.ko

  4. netlink_multicast 需要驱动层打开 NETLINK_MULTICAST 宏定义

     Compile kernel module and user space program.
     make

     Load kernel module:
     insmod ./netlink_test.ko

     Also check kernel log `dmesg` for module debug output.
     ./nl_recv "Hello you!"
     Listen for message...
     Received from kernel: Hello you!
     Listen for message...

     Execute `./nl_recv` in another console as well and see how the message is send to the kernel and back to all running nl_recv instances.
     Note: Only root or the kernel can send a message to a multicast group!

     Unload kernel module:
     rmmod netlink_test.ko

  5. netlink_namespace 需要驱动层打开 NETLINK_NAMESPACE 宏定义

     Compile kernel module and user space program.
     make

     Load kernel module:
     insmod ./netlink_test.ko

     Check kernel log `dmesg` for module debug output.
     ./nl_recv "Hello you!"
     Hello you!

     Unload kernel module:
     rmmod netlink_test.ko

# netfilter
  这个模块是独立的，和其它目录和模块没任何关系，单独运行  
  演示了对链路层网络数据包的过滤过程，有兴趣的小伙伴可以自我学习一下, 网上相关的内容及完整的代码相当少。

# dev_drv
  这个模块是独立的，和其它目录和模块没任何关系，单独运行  
  讲解了有关 device, bus, driver 之间的关系，并提供了对应的小例子

# 更多
  更多内容，可以关注我的 github :  
  https://github.com/superconvert

