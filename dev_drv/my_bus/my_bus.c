#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

//------------------------------------------------------------------------------------------------
static char* my_bus_name = "my_bus";
static char my_bus_rwbuf[128] = "readwrite-default";

static ssize_t bus_attrs_version_show(
    struct bus_type* bus,
    char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: version 1.0.0\n", my_bus_name);
}

static ssize_t bus_attrs_rw_show(
    struct bus_type* bus,
    char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: %s\n", my_bus_name, my_bus_rwbuf);
}

static ssize_t bus_attrs_rw_store(
    struct bus_type* bus,
    const char* buf,
    size_t count)
{
    if ( count >= 128 ) {
        count = 128;
    }
    if ( buf ) {
        memcpy(my_bus_rwbuf, buf, count);
        my_bus_rwbuf[count] = 0;
    }
    return count;
}

static BUS_ATTR(version, S_IRUGO, bus_attrs_version_show, NULL);
static BUS_ATTR(rwstate, (S_IRUGO | S_IWUGO), bus_attrs_rw_show, bus_attrs_rw_store);

static struct attribute* my_bus_attrs[] = {
    &bus_attr_version.attr,
    &bus_attr_rwstate.attr,
    NULL,
};

//------------------------------------------------------------------------------------------------
static char* my_device_name = "my_device";
static char my_device_rwbuf[128] = "readwrite-default";

static ssize_t device_attrs_version_show(
    struct device* dev,
    struct device_attribute* attr,
    char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: version 1.0.0\n", my_device_name);
}

static ssize_t device_attrs_rw_show(
    struct device* dev,
    struct device_attribute* attr,
    char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: %s\n", my_device_name, my_device_rwbuf);
}

static ssize_t device_attrs_rw_store(
    struct device* dev,
    struct device_attribute* attr,
    const char* buf,
    size_t count)
{
    if (count >= 128) {
        count = 128;
    }
    if (buf) {
        memcpy(my_device_rwbuf, buf, count);
        my_device_rwbuf[count] = 0;
    }
    return count;
}

static DEVICE_ATTR(version, S_IRUGO, device_attrs_version_show, NULL);
static DEVICE_ATTR(rwstate, (S_IRUGO | S_IWUGO), device_attrs_rw_show, device_attrs_rw_store);

static struct attribute* my_dev_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_rwstate.attr,
    NULL,
};

//------------------------------------------------------------------------------------------------
static char* my_driver_name = "my_driver";
static char my_driver_rwbuf[128] = "readwrite-default";

static ssize_t driver_attrs_version_show(
    struct device_driver* driver,
    char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: version 1.0.0\n", my_driver_name);
}

static ssize_t driver_attrs_rw_show(
    struct device_driver* driver,
    char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: %s\n", my_driver_name, my_driver_rwbuf);
}

static ssize_t driver_attrs_rw_store(
    struct device_driver* driver,
    const char* buf,
    size_t count)
{
    if (count >= 128) {
        count = 128;
    }
    if (buf) {
        memcpy(my_driver_rwbuf, buf, count);
        my_driver_rwbuf[count] = 0;
    }
    return count;
}

#ifndef DRIVER_ATTR
#define DRIVER_ATTR(_name, _mode, _show, _store) \
        struct driver_attribute driver_attr_##_name = __ATTR(_name, _mode, _show, _store)
#endif

static DRIVER_ATTR(version, S_IRUGO, driver_attrs_version_show, NULL);
static DRIVER_ATTR(rwstate, (S_IRUGO | S_IWUGO), driver_attrs_rw_show, driver_attrs_rw_store);

static struct attribute* my_drv_attrs[] = {
    &driver_attr_version.attr,
    &driver_attr_rwstate.attr,
    NULL,
};

int my_match(
    struct device* dev,
    struct device_driver* drv)
{
    // 比较设备的 bus_id 与驱动的名字是否匹配，匹配一致则在 insmod 驱动 时候调用 probe 函数 
    return !strncmp(dev->kobj.name, drv->name, strlen(drv->name));
}

static const struct attribute_group bus_attr_group = {
    .attrs = my_bus_attrs,
};

static const struct attribute_group dev_attr_group = {
    .attrs = my_dev_attrs,
};

static const struct attribute_group drv_attr_group = {
    .attrs = my_drv_attrs,
};

const struct attribute_group* my_bus_groups[] = {
    &bus_attr_group,
    NULL,
};

const struct attribute_group* my_dev_groups[] = {
    &dev_attr_group,
    NULL,
};

const struct attribute_group* my_drv_groups[] = {
    &drv_attr_group,
    NULL,
};

struct bus_type my_bus_type = {
    .name = "my_bus",
    .match = my_match,
    .bus_groups = my_bus_groups,
    .dev_groups = my_dev_groups,
    .drv_groups = my_drv_groups,
};

static int __init my_bus_init(void)
{
    int ret;
    ret = bus_register(&my_bus_type);
    return ret;
}

static void __exit my_bus_exit(void)
{
    bus_unregister(&my_bus_type);
}

MODULE_AUTHOR("freeabc");
MODULE_DESCRIPTION("my_bus demo!");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(my_bus_type);
module_init(my_bus_init);
module_exit(my_bus_exit);
