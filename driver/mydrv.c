/*
 * 驱动常用技术模型演示
 * 
 * 功能基本说明:
 * 演示了一个驱动对应多个设备，以及对具体设备的存取
 * 演示了应用与驱动，mmap 的映射方式的实现与访问
 * 演示了应用层通过 select, poll, epoll 驱动层的实现, 执行 read 和 write 程序
 * 演示了 netlink 的单播，多播，命名空间的使用方式, 执行 unicast 和 multicast, namespace 程序
 *
 * 驱动参数可以通过下面的方式进行设置:
 * insmod mydrv.ko mode=1 port=1,2,3,4 name="mydrv"
 *
 * 查看驱动日志打印:
 * dmesg -w
 *
 * 查看驱动信息:
 * modinfo mydrv.ko 
 *
 * 常用的驱动命令:
 * insmod, lsmod, rmmod, mknod, depmod, modprobe
 * mknod /dev/xxx c [主设备号] [次设备号]
 *
 * 其它相关点:
 * 查看设备信息 cat /proc/devices
 * 查看设备树   ls /proc/device-tree
 * 查看设备文件 ls /dev
 *
 * 更多内容，可以关注我的 github :
 * https://github.com/superconvert
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/netlink.h>
#include <linux/pid_namespace.h>
#include <net/sock.h> 
#include <net/net_namespace.h>
#include <net/netns/generic.h>

#include "mydrv.h"

// 驱动名称
#define mydrv_name "mydrv"

// 开启 compat 接口
#define CONFIG_COMPAT 1

// 开启 mmap 演示
#define MMAP_DEMO 1

// 开启驱动线程演示
#define THREAD_DEMO 0

// netlink 单播演示
#define NETLINK_UNICAST 0

// netlink 多播演示
#define NETLINK_MULTICAST 0

// netlink 命名空间演示
#define NETLINK_NAMESPACE 0

// 设备的主设备号
#ifndef MYDRV_MAJOR
#define MYDRV_MAJOR 0
#endif

// 开启的设备数
#ifndef MYDRV_NR_DEVS
#define MYDRV_NR_DEVS 2
#endif

// 开启的空间大小
#ifndef MYDRV_SIZE
#define MYDRV_SIZE 4096
#endif

// seek 操作定义
#define MYSEEK_SET 0
#define MYSEEK_CUR 1
#define MYSEEK_END 2

//------------------------------------------------------------------------------------------
//
// mydrv 设备描述结构体
//
//------------------------------------------------------------------------------------------
struct mydrv_dev
{                                                        
    // 设备编号
    int seq;
    // 设备对象
    struct cdev cdev;
    // mmap 演示对象
#ifdef MMAP_DEMO
    char *mmap_buffer;
#endif

    //
    // select, poll, epoll 演示
    //
    bool has_data;
    int data_size; 
    char *data_buffer;
    wait_queue_head_t wait_data; 

    // 
    // ioctl 演示对象定义
    //
    int ioctl_value;
    char ioctl_buffer[256];
    struct myioctl_data ioctl_data;
};

static int major = MYDRV_MAJOR;
static int mode = 0;
static int port[4] = {0,1,2,3};
static char *name = "demo";

module_param(major, int, S_IRUGO);
module_param(mode, int, S_IRUGO);
module_param(name, charp, S_IRUGO);
module_param_array(port, int, NULL, S_IRUGO);

// 设备号
static dev_t mydrv_devno;

// select, poll, epoll 演示
static struct class *mydrv_class = NULL;

// 设备结构体指针
static struct mydrv_dev *mydrv_devp = NULL;

// netlink 单播定义
#if NETLINK_UNICAST
  #define NETLINK_TEST 17
  static struct sock *nl_unicast = NULL;
#endif

// netlink 多播定义
#if NETLINK_MULTICAST
  #define MYGRP 17
  #define MYPROTO NETLINK_USERSOCK
  static struct sock *nl_multicast = NULL;
#endif

// netlink 命名空间定义
#if NETLINK_NAMESPACE
  #define NETLINK_TEST 17
  // network namespace index, set once on module load
  static unsigned int net_id;
  // a different netlink socket for every network namespace
  struct ns_data {
      struct sock *sk;
  };
#endif

// 线程演示定义
#if THREAD_DEMO
  static struct task_struct *test_task = NULL;
#endif

//------------------------------------------------------------------------------------------
//
// 设备打开函数
//
//------------------------------------------------------------------------------------------
static inline int mydrv_open(
    struct inode *inode,
    struct file *filp)
{
    struct mydrv_dev *dev;
    // 获取次设备号
    int num = MINOR(inode->i_rdev);
    if ( num >= MYDRV_NR_DEVS ) { 
	printk(KERN_ERR "%s : open device mydrv%d failed.\n", mydrv_name, num);
        return -ENODEV;
    }
    dev = &mydrv_devp[num];
    printk(KERN_INFO "%s : open device mydrv%d success.\n", mydrv_name, num);
    // 将设备描述结构指针赋值给文件私有数据指针
    filp->private_data = dev;
    
    return 0; 
}

//------------------------------------------------------------------------------------------
//
// 设备释放函数
//
//------------------------------------------------------------------------------------------
static inline int mydrv_release(
    struct inode *inode,
    struct file *filp)
{
    struct mydrv_dev *dev = filp->private_data;
    printk(KERN_INFO "%s : release device mydrv%d success.\n", mydrv_name, dev->seq);
    return 0;
}

//------------------------------------------------------------------------------------------
//
// 读函数
//
//------------------------------------------------------------------------------------------
static inline ssize_t mydrv_read(
    struct file *filp,
    char __user *buf,
    size_t size,
    loff_t *ppos)
{
    int ret = 0;
    unsigned long p =  *ppos;
    unsigned int count = size;
    struct mydrv_dev *dev = filp->private_data;

    // 判断读位置是否有效
    if ( p >= MYDRV_SIZE ) {
        return 0;
    }

    if ( count > MYDRV_SIZE - p ) {
        count = MYDRV_SIZE - p;
    }
    
    while ( !dev->has_data ) {
        if ( filp->f_flags & O_NONBLOCK ) {
            return -EAGAIN;
        }
        wait_event_interruptible(dev->wait_data, dev->has_data);
    }

    // 读数据到用户空间
    if ( copy_to_user(buf, (void*)(dev->data_buffer + p), count) ) {
        ret =  - EFAULT;
    } else {
        *ppos += count;
        ret = count;
        printk(KERN_INFO "%s : read device mydrv%d %d bytes(s) from %lu\n", mydrv_name, dev->seq, count, p);
    }
  
    // 表明不再有数据可读
    dev->has_data = false;
    // 唤醒写进程 
    return ret;
}

//------------------------------------------------------------------------------------------
//
// 写函数
//
//------------------------------------------------------------------------------------------
static inline ssize_t mydrv_write(
    struct file *filp,
    const char __user *buf,
    size_t size,
    loff_t *ppos)
{
    unsigned long p =  *ppos;
    unsigned int count = size;
    int ret = 0;
    struct mydrv_dev *dev = filp->private_data;
  
    // 分析和获取有效的写长度
    if ( p >= MYDRV_SIZE ) {
        return 0;
    }

    if ( count > MYDRV_SIZE - p ) {
        count = MYDRV_SIZE - p;
    }

    // 从用户空间写入数据
    if ( copy_from_user(dev->data_buffer + p, buf, count) ) {
        ret =  - EFAULT;
    } else {
        *ppos += count;
        ret = count;
        printk(KERN_INFO "%s : written device mydrv%d %d bytes(s) from %lu\n", mydrv_name, dev->seq, count, p);
    }
  
    // 有新的数据可读
    dev->has_data = true;
    // 唤醒读进程
    wake_up(&(dev->wait_data));

    return ret;
}

//------------------------------------------------------------------------------------------
//
// ioctl 64 位函数定义
//
// compat_ioctl 这个函数是为了使 32 位的应用程序可以与 64 位的内核驱动兼容而产生的。 
//
// 最主要实现的功能就是：
//   将应用层32位的数据转成64位传给64位内核驱动。
//   将内核驱动64位的数据转成32位传给32位内核驱动。
//
// 所以调用关系如下：
//   当应用层是 32 位程序，内核及架构是 32 位程序，那么驱动的 unlocked_ioctl 函数被调用。
//   当应用层是 32 位程序，内核及架构是 64 位程序，那么驱动的 compat_ioctl 函数被调用。
//   当应用层是 64 位程序，内核及架构是 64 位程序，那么驱动的 unlocked_ioctl 函数被调用。
//
//------------------------------------------------------------------------------------------
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static inline int mydrv_ioctl(
    struct inode *indoe,
    struct file *filp,
    unsigned int cmd,
    unsigned long arg)
{
#else
static inline long mydrv_unlocked_ioctl(
    struct file *filp,
    unsigned int cmd,
    unsigned long arg)
{
    struct inode *inode = inode = file_inode(filp);
#endif

    // 获得设备结构体指针 
    struct mydrv_dev *dev = filp->private_data;

    if ( _IOC_TYPE(cmd) != MYIOCTL_MAGIC ) {
        printk(KERN_ERR "%s : bad ioctl cmd type %c:%d\n", mydrv_name, _IOC_TYPE(cmd), cmd);
        return -ENOTTY;
    }

    switch ( cmd ) {
      case MYIOCTL_SET_INT:
        if ( copy_from_user(&(dev->ioctl_value), (void *)arg, _IOC_SIZE(cmd)) ) {
            return -EFAULT;
        }
        printk(KERN_INFO "%s : ioctl is set int %d\n", mydrv_name, dev->ioctl_value);
        break;

      case MYIOCTL_GET_INT:
        dev->ioctl_value += 1;
        if ( copy_to_user((void *)arg, &(dev->ioctl_value), sizeof(int)) ) {
            return -EFAULT;
        }
        printk(KERN_INFO "%s : ioctl is get int %d\n", mydrv_name, dev->ioctl_value);
        break;

      case MYIOCTL_SET_STR:
        if ( copy_from_user(dev->ioctl_buffer, (void *)arg, _IOC_SIZE(cmd)) ) {
            return -EFAULT;
        }
        printk(KERN_INFO "%s : ioctl is set str %s\n", mydrv_name, dev->ioctl_buffer);
        break;

      case MYIOCTL_GET_STR:
        strcpy(dev->ioctl_buffer, "hello app string");
        if ( copy_to_user((void *)arg, dev->ioctl_buffer, strlen(dev->ioctl_buffer)) ) {
            return -EFAULT;
        }
        printk(KERN_INFO "%s : ioctl is get str %s\n", mydrv_name, dev->ioctl_buffer);
        break;

      case MYIOCTL_SET_OBJ:
        if ( copy_from_user(&(dev->ioctl_data), (struct myioctl_data __user *)arg, sizeof(struct myioctl_data)) ) {
            return -EFAULT;
        }
        printk(KERN_INFO "%s : ioctl is set obj ok\n", mydrv_name);
        break;

      case MYIOCTL_GET_OBJ:
        dev->ioctl_data.i_data += 1;
        strcpy(dev->ioctl_data.s_data, "hello app object");
        if ( copy_to_user((struct myioctl_data __user *)arg, &(dev->ioctl_data), sizeof(struct myioctl_data)) ) {
            return -EFAULT;
        }
        printk(KERN_INFO "%s : ioctl is get obj ok\n", mydrv_name);
        break;

      default:
    	printk(KERN_INFO "%s : ioctl cmd is error\n", mydrv_name);
        return  -EINVAL;
    }
    return 0;
}

#if CONFIG_COMPAT
//------------------------------------------------------------------------------------------
//
// ioctl 32 位函数定义
//
//------------------------------------------------------------------------------------------
static inline long mydrv_compat_ioctl(
    struct file *filp,
    unsigned int cmd,
    unsigned long arg)
{
    return mydrv_unlocked_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

//------------------------------------------------------------------------------------------
//
// seek文件定位函数 
//
//------------------------------------------------------------------------------------------
static inline loff_t mydrv_llseek(
    struct file *filp,
    loff_t offset,
    int whence)
{ 
    loff_t newpos;

    switch ( whence ) {
      case MYSEEK_SET: 
        newpos = offset;
        break;

      case MYSEEK_CUR:
        newpos = filp->f_pos + offset;
        break;

      case MYSEEK_END:
        newpos = MYDRV_SIZE -1 + offset;
        break;

      default: /* can't happen */
        return -EINVAL;
    }

    if ( (newpos<0) || (newpos>MYDRV_SIZE) ) {
        return -EINVAL;
    }
        
    filp->f_pos = newpos;

    return newpos;
}

//------------------------------------------------------------------------------------------
//
// mmap 的实现
//
//------------------------------------------------------------------------------------------
static inline int mydrv_mmap(
    struct file *filp,
    struct vm_area_struct *vma)
{
    struct mydrv_dev *dev = filp->private_data;
#if MMAP_DEMO
    unsigned long page;
    unsigned long start = (unsigned long)vma->vm_start;
    //unsigned long end =  (unsigned long)vma->vm_end;
    unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);
 
    // 得到物理地址
    page = virt_to_phys(dev->mmap_buffer);    
    // 将用户空间的一个vma虚拟内存区映射到以page开始的一段连续物理页面上, 
    // 第三个参数是页帧号，由物理地址右移 PAGE_SHIFT 得到
    if ( remap_pfn_range( vma, start, page>>PAGE_SHIFT, size, PAGE_SHARED) ) {
        return -1;
    }

    // 往该内存写 "hello mmap" 
    strcpy(dev->mmap_buffer, "hello mmap");
#endif

    return 0;
}

//------------------------------------------------------------------------------------------
//
// poll 满足应用层用 select, poll, epoll_wait 调用
//
//------------------------------------------------------------------------------------------
static inline unsigned int mydrv_poll(
    struct file *filp,
    poll_table *wait)
{
    struct mydrv_dev *dev = filp->private_data; 
    unsigned int mask = 0;
    
    // 将等待队列添加到 poll_table
    poll_wait(filp, &dev->wait_data,  wait);
    printk(KERN_INFO "%s : poll device mydrv%d ok\n", mydrv_name, dev->seq);

    // readable
    if ( dev->has_data ) {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

//------------------------------------------------------------------------------------------
//
// 设备对象释放
//
//------------------------------------------------------------------------------------------
static inline int cdev_release(int num)
{
    int i;
    for ( i = 0; i < num; i++ ) {
    #if MMAP_DEMO
        // 清除保留
        ClearPageReserved(virt_to_page(mydrv_devp[i].mmap_buffer));
        // 释放内存
        kfree(mydrv_devp[i].mmap_buffer);
    #endif
        device_destroy(mydrv_class, mydrv_devno+i);
        cdev_del(&(mydrv_devp[i].cdev));
    }
    return 0;
}

//------------------------------------------------------------------------------------------
//
// 线程定义函数
//
//------------------------------------------------------------------------------------------
#if THREAD_DEMO
int thread_proc(void *data) {

    // 给线程命名
    // daemonize("mydrv_thread");

    while ( 1 ) {
        // set_current_state(TASK_UNINTERRUPTIBLE);
        if ( kthread_should_stop() ) {
            break;
        }

        if ( 1 ) { 
            // 进行业务处理
            schedule_timeout(HZ);
        } else {
            // 让出CPU运行其他线程，并在指定的时间内重新被调度
            schedule_timeout(HZ);
        }

    }

    return 0;
}
#endif

//------------------------------------------------------------------------------------------
//
// netlink 单播模式
//
//------------------------------------------------------------------------------------------
#if NETLINK_UNICAST
static inline void netlink_unicast_recv_msg(struct sk_buff *skb)
{
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size;
    char *msg;
    int pid;
    int res;

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid; /* pid of sending process */
    msg = (char *)nlmsg_data(nlh);
    msg_size = strlen(msg);

    printk(KERN_INFO "%s : netlink unicast received from pid %d: %s\n", mydrv_name, pid, msg);

    // create reply
    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out) {
        printk(KERN_ERR "%s : netlink unicast failed to allocate new skb\n", mydrv_name);
        return;
    }

    // put received message into reply
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    printk(KERN_INFO "%s : netlink unicast send %s\n", mydrv_name, msg);

    res = nlmsg_unicast(nl_unicast, skb_out, pid);
    if ( res < 0 ) {
        printk(KERN_ERR "%s : netlink unicast error while sending skb to user\n", mydrv_name);
    }
}
#endif

//------------------------------------------------------------------------------------------
//
// netlink 多播模式
//
//------------------------------------------------------------------------------------------
#if NETLINK_MULTICAST
static inline void netlink_multicast_recv_msg(struct sk_buff *skb)
{
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size;
    char *msg;
    int pid;
    int res;

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid; /* pid of sending process */
    msg = (char *)nlmsg_data(nlh);
    msg_size = strlen(msg);
    printk(KERN_INFO "%s : netlink multicast received from pid %d: %s\n", mydrv_name, pid, msg);

    // create reply
    skb_out = nlmsg_new(msg_size, 0);
    if ( !skb_out ) {
        printk(KERN_ERR "%s : netlink multicast failed to allocate new skb\n", mydrv_name);
        return;
    }

    // put received message into reply
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    //NETLINK_CB(skb_out).dst_group = MYGRP; /* in multicast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    printk(KERN_INFO "%s : netlink multicast send %s\n", mydrv_name, msg);
    res = nlmsg_multicast(nl_multicast, skb_out, 0, MYGRP, GFP_KERNEL);

    if ( res < 0 ) {
        printk(KERN_ERR "%s : netlink multicast error while sending skb to user\n", mydrv_name);
    }
}
#endif

//------------------------------------------------------------------------------------------
//
// netlink 命名空间模式
//
//------------------------------------------------------------------------------------------
#if NETLINK_NAMESPACE
static inline void netlink_ns_recv_msg(struct sk_buff *skb)
{
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size;
    char *msg;
    int pid;
    int res;

    nlh = (struct nlmsghdr *)skb->data;
    msg = (char *)nlmsg_data(nlh);
    msg_size = strlen(msg);
    printk(KERN_INFO "%s : netlink namespace received %s\n", mydrv_name, msg);

    // pid of the sending (user space) process
    pid = nlh->nlmsg_pid;

    // get namespace of the sending process
    struct net *net = get_net_ns_by_pid(pid);

    printk(KERN_INFO "%s : netlink namespace received from pid %d, namespace %p, net_id: %d: %s\n",
        mydrv_name, pid, net, net_id, msg);

    // get our data for this network namespace
    struct ns_data *data = net_generic(net, net_id);
    if ( data == NULL || data->sk == NULL ) {
        printk(KERN_ERR "%s : netlink namespace data or socket is NULL\n", mydrv_name);
        return;
    }

    // create reply
    skb_out = nlmsg_new(msg_size, 0);
    if ( !skb_out ) {
        printk(KERN_ERR "%s : netlink namespace failed to allocate new skb\n", mydrv_name);
        return;
    }

    // put message into response
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    printk(KERN_INFO "%s : netlink namespace send %s\n", mydrv_name, msg);

    res = nlmsg_unicast(data->sk, skb_out, pid);
    if ( res < 0 ) {
        printk(KERN_ERR "%s : netlink namespace error while sending skb to user\n", mydrv_name);
    }
}

//------------------------------------------------------------------------------------------
//
// Called for every existing and added network namespaces
//
//------------------------------------------------------------------------------------------
static int __net_init ns_netlink_test_init(struct net *net)
{
    struct netlink_kernel_cfg cfg = {
        .input = netlink_ns_recv_msg,
        .flags = NL_CFG_F_NONROOT_RECV,
    };

    // create netlink socket
    struct sock *nl_sock = netlink_kernel_create(net, NETLINK_TEST, &cfg);
    if ( !nl_sock ) {
        printk(KERN_ALERT "%s : netlink namespace test error creating socket.\n", mydrv_name);
        return -ENOMEM;
    }

    // create data item in network namespace (net) under the id (net_id) 
    struct ns_data *data = net_generic(net, net_id);
    data->sk = nl_sock;

    return 0;
}

//------------------------------------------------------------------------------------------
//
// netlink test exit
//
//------------------------------------------------------------------------------------------
static void __net_exit ns_netlink_test_exit(struct net *net)
{
    // called when the network namespace is removed
    struct ns_data *data = net_generic(net, net_id);

    // close socket
    netlink_kernel_release(data->sk);
}

//------------------------------------------------------------------------------------------
//
// callback to make the module network namespace aware
//
//------------------------------------------------------------------------------------------
static struct pernet_operations net_ops __net_initdata = {
    .id = &net_id,
    .init = ns_netlink_test_init,
    .exit = ns_netlink_test_exit,
    .size = sizeof(struct ns_data),
};
#endif

//------------------------------------------------------------------------------------------
//
// 文件操作结构体
//
//------------------------------------------------------------------------------------------
static struct file_operations mydrv_fops =
{
    .owner = THIS_MODULE,
    .open = mydrv_open,
    .poll = mydrv_poll,
    .mmap = mydrv_mmap,
    .read = mydrv_read,
    .write = mydrv_write,
    .llseek = mydrv_llseek,
    .release = mydrv_release,
#if CONFIG_COMPAT
    .compat_ioctl = mydrv_compat_ioctl,
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    .ioctl = mydrv_ioctl,
#else
    .unlocked_ioctl = mydrv_unlocked_ioctl,
#endif
};

//------------------------------------------------------------------------------------------
//
// 设备驱动模块加载函数
//
//------------------------------------------------------------------------------------------
static inline int mydrv_init(void)
{
    int i, result;
    struct device *mydrv_device = NULL;

#if THREAD_DEMO
    test_task = kthread_create(thread_proc, NULL, "mydrv_trhead");
    if ( IS_ERR(test_task) ) {
        printk(KERN_ALERT "%s : unable to start kernel thread.\n", mydrv_name);
        test_task =NULL;
        return EBUSY;
    }
    wake_up_process(test_task);
#endif

#if NETLINK_UNICAST
    struct netlink_kernel_cfg cfg_uni = {
        .input = netlink_unicast_recv_msg,
    };
    nl_unicast = netlink_kernel_create(&init_net, NETLINK_TEST, &cfg_uni);
    if ( !nl_unicast ) {
        printk(KERN_ALERT "%s : netlink unicast error creating socket.\n", mydrv_name);
        return EBUSY;
    }
#endif

#if NETLINK_MULTICAST
    struct netlink_kernel_cfg cfg_multi = {
        .input = netlink_multicast_recv_msg,
    };
    nl_multicast = netlink_kernel_create(&init_net, MYPROTO, &cfg_multi);
    if ( !nl_multicast ) {
        printk(KERN_ALERT "%s : netlink multicast error creating socket.\n", mydrv_name);
        return EBUSY;
    }
#endif

#if NETLINK_NAMESPACE
    register_pernet_subsys(&net_ops);
#endif

    // 主设备号用来区分不同种类的设备，而次设备号用来区分同一类型的多个设备
    // 例如一个嵌入式系统，有两个LED指示灯，LED灯需要独立的打开或者关闭。那么，
    // 可以写一个LED灯的字符设备驱动程序，可以将其主设备号注册成5号设备，次设备号分别为1和2
    // 这里的 MYDRV_NR_DEVS 是一个驱动对应多少个设备
    mydrv_devno = MKDEV(major, 0);
    if ( major ) {
        // 静态申请设备号
        result = register_chrdev_region(mydrv_devno, 2, mydrv_name);
    } else {
        // 动态分配设备号
        result = alloc_chrdev_region(&mydrv_devno, 0, 2, mydrv_name);
        major = MAJOR(mydrv_devno);
    }

    if ( result < 0 ) {
        result = -ENOMEM;
        printk(KERN_ERR "%s : [register|alloc]_chrdev_region failed %d\n", mydrv_name, result);
        return result;
    }
    
    // 没有下面这两句话话，不会产生 /dev/mydrv
    mydrv_class = class_create(THIS_MODULE, mydrv_name);
    if ( IS_ERR(mydrv_class) ) {
        result = -EAGAIN;
        printk(KERN_ERR "%s : class_create failed\n", mydrv_name);
        goto fail_class_create;
    }

    // 为设备描述结构分配内存
    mydrv_devp = kmalloc(MYDRV_NR_DEVS * sizeof(struct mydrv_dev), GFP_KERNEL);
    if ( !mydrv_devp ) {
        result = -ENOMEM;
        printk(KERN_ERR "%s : kmalloc for devp failed\n", mydrv_name);
        goto fail_malloc;
    }
    memset(mydrv_devp, 0, MYDRV_NR_DEVS * sizeof(struct mydrv_dev));

    // 为设备分配内存
    for ( i = 0; i < MYDRV_NR_DEVS; i++ ) {
        char name[64];
        int tmp_devno = mydrv_devno + i;

        // 初始化cdev结构
        cdev_init(&(mydrv_devp[i].cdev), &mydrv_fops);
        mydrv_devp[i].cdev.owner = THIS_MODULE;
        mydrv_devp[i].cdev.ops = &mydrv_fops;
    
        // 注册字符设备, cdev_add 函数最后一个参数 count 详解
        // count 是应当关联到设备的设备号的数目. 常常 count 是 1, 但是有多个设备号对应于一个特定的设备的情形. 
        // 例如, 设想 SCSI 磁带驱动, 它允许用户空间来选择操作模式(例如密度), 通过安排多个次编号给每一个物理设备
        // 这里的 count 是一个设备 由几个设备号共同使用
        sprintf(name, "%s%d", mydrv_name, i);
        result = cdev_add(&(mydrv_devp[i].cdev), tmp_devno, 1);
        if ( result ) {
            printk(KERN_ERR "%s : cdev_add %s failed %d\n", mydrv_name, name, result);
            //device_destroy(mydrv_class, tmp_devno);
            goto fail_cdev_add;
        }

        mydrv_device = device_create(mydrv_class, NULL, tmp_devno, NULL, name);
        if ( IS_ERR(mydrv_device) ) {
            printk(KERN_ERR "%s : device_create %s failed\n", mydrv_name, name);
            cdev_del(&(mydrv_devp[i].cdev));
            goto fail_device_create;
        }
        printk(KERN_INFO "%s : create device %s success.\n", mydrv_name, name);

    #if MMAP_DEMO
        // 内存分配
        mydrv_devp[i].mmap_buffer = (unsigned char *)kmalloc(PAGE_SIZE,GFP_KERNEL);
        // 将该段内存设置为保留
        SetPageReserved(virt_to_page(mydrv_devp[i].mmap_buffer));
    #endif

        mydrv_devp[i].seq = i; 
        mydrv_devp[i].ioctl_value = 0; 
        mydrv_devp[i].has_data = false;
        mydrv_devp[i].data_size = MYDRV_SIZE;
        mydrv_devp[i].data_buffer = kmalloc(MYDRV_SIZE, GFP_KERNEL);
        memset(mydrv_devp[i].data_buffer, 0, MYDRV_SIZE);
        memset(mydrv_devp[i].ioctl_buffer, 0, 256);
        memset(&(mydrv_devp[i].ioctl_data), 0, sizeof(struct myioctl_data));
        // 初始化等待队列
        init_waitqueue_head(&(mydrv_devp[i].wait_data));
    }

    printk(KERN_INFO "%s : init ok\n", mydrv_name);

    return 0;

fail_cdev_add :
fail_device_create :
    cdev_release(i);
    class_destroy(mydrv_class);

fail_malloc :
    device_destroy(mydrv_class, mydrv_devno);

fail_class_create :
    unregister_chrdev_region(mydrv_devno, MYDRV_NR_DEVS);
    
    return result;
}

//------------------------------------------------------------------------------------------
//
// 模块卸载函数
//
//------------------------------------------------------------------------------------------
static void mydrv_exit(void)
{
#if THREAD_DEMO
    if ( test_task ) {
        kthread_stop(test_task);
        test_task = NULL;
    }
#endif

#if NETLINK_UNICAST
    netlink_kernel_release(nl_unicast);
#endif

#if NETLINK_MULTICAST
    netlink_kernel_release(nl_multicast);
#endif

#if NETLINK_NAMESPACE
    unregister_pernet_subsys(&net_ops);
#endif

    // 释放设备
    cdev_release(MYDRV_NR_DEVS);
    // 注销类
    class_destroy(mydrv_class);
    // 释放设备结构体内存
    kfree(mydrv_devp);
    // 释放设备号
    unregister_chrdev_region(mydrv_devno, MYDRV_NR_DEVS);

    printk(KERN_INFO "%s : exit ok\n", mydrv_name);
}

//------------------------------------------------------------------------------------------
//
// insmod 或 modprobe 命令会最终调用模块装载
// rmmod 或 modprobe -r 命令最终调用模块卸载
//
//------------------------------------------------------------------------------------------
module_init(mydrv_init);
module_exit(mydrv_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0.0");
MODULE_AUTHOR("SuperConvert");
MODULE_DESCRIPTION("Driver mode demo!");
