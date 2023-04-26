# 介绍 
  1. 演示了一个驱动对应多个设备，以及对具体设备的存取
  2. 演示了应用与驱动，mmap 的映射方式的实现与访问
  3. 演示了应用层通过 select, poll, epoll 驱动层的实现, 执行 read 和 write 程序
  4. netlink 的单播，多播，命名空间的使用方式, 执行 unicast 和 multicast, namespace 程序

# driver
  1. 删除驱动 rmmod mydrv.ko
  2. 一般内核都会对模块进行签名检查，没好的办法，关闭内核源码的对模块签名，重新编译内核，用新内核启动 CONFIG_MODULE_SIG=n

# app

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

