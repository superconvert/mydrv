#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

// cat /proc/kallsyms | grep my_bus_type
extern struct bus_type my_bus_type;

int my_probe(struct device *dev)
{
    printk("my_driver found the device it can handle!\n");
    return 0;
}

struct device_driver my_driver = {
    .name = "my_driver",
    .bus = &my_bus_type, // 指明这个驱动程序是属于my_bus_type这条总线上的设备
    .probe = my_probe,   // 在my_bus_type总线上找到它能够处理的设备，就调用my_probe;删除设备调用my_remove
};

/**
 * 能够处理的设备? 
 * 1、什么时候驱动程序从总线找能够处理的设备；
 *    答：驱动注册时候
 * 2、凭什么说能够处理呢？（或者说标准是什么）；
 *    答：驱动与设备都是属于总线上的，利用总线的结构bus_type中match函数
**/
static int __init my_driver_init(void)
{
    int ret;
    ret = driver_register(&my_driver);
    return ret;
}

static void __exit my_driver_exit(void)
{
    driver_unregister(&my_driver);  
}

MODULE_AUTHOR("freeabc");
MODULE_DESCRIPTION("my_driver demo!");
MODULE_LICENSE("GPL");

module_init(my_driver_init);
module_exit(my_driver_exit);
