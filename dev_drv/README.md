# 目的
* 初步了解 linux 设备驱动框架模型
* 初步了解设备驱动模型有哪些元素
* 设备驱动模型元素的说明及解释
* 设备驱动模型元素的工作原理
* 设备驱动模型的小例子

对整体有个粗略的了解，设备驱动类型种类太多，不同的设备具有不同的特性，如果想完全掌握这块知识，不是一朝一夕的事情，更不是一两篇博客搞定的事情，但入门概念和设计思路一旦了解清楚，后续的工作将会大大加速。

# 概述
说到 linux 设备驱动模型，有几个概念不得不提
* bus
* class
* device
* driver

我们使用设备和驱动的场景不同，就要求我们从多角度的视角去理解它们。有的是为了管理上方便，维护上方便，易读性，所以才有了上述的概念。一个驱动可能对应多个设备，一个设备（拥有多种能力）也可能对应多个驱动。驱动和设备又可以分类，设备与驱动又可以通过 bus 集中管理

# bus 的由来
设备种类异常繁多，不同厂商同一类设备还有不同版本；潮起潮落，有的老厂商死去，有新厂商进来，要面对这么多复杂的变化，对应代码的维护是非常有挑战性的。早期的 linux 把设备驱动维护全部放入内核。由于设备的变化，内核对应的代码也不得不做出同样的变化，这给内核的稳定性，维护性，易用性提出了极大的挑战。所有问题都是因为驱动要拿到设备的基地址、中断号，这些信息针对不同的设备是不断变化的。早期的驱动基本上是针对固定硬件的，这些信息都是 hard code 到驱动里面的，一旦设备有新增或更改，对应的驱动部分也需要进行调整，这就造成了驱动和设备的耦合性太强，为了解决这个问题，需要给驱动和设备加一个适配层对其之间的关系进行解耦，这一层就是 bus 的由来。bus 负责 device 和 driver 的管理，解耦了 device 和 driver 的绑定关系。device 可以动态的把信息注册到 bus 上，driver 同样也是需要注册到 bus 上，这样做的好处是 driver 可以动态的拿取对应的设备的基地址、中断号，这样无论设备怎么变化，驱动所对应的实现基本上不需要更改了。同时 bus 也具有天然的分类功能，成千上万的设备，层出不穷，我们集中管理非常不方便，效率低下，有了bus 就具有了天然的对设备进行分组的概念，益处是显而易见的。同时，现在的硬件体系很多设备是挂在对应的总线上，也符合理解的习惯，硬件体系中还有很多设备并没有挂到真正的总线上，对于这类设备的管理系统会虚拟出一条总线对它们进行统一管理，这就是 platform bus 的由来。下面再提一下三种特殊的 bus

* 三种特殊的 bus （ 这篇文章写的比较好，下面的一小段话，直接摘抄过来了，对此表示感谢。 原文链接：https://blog.csdn.net/yangjizhen1533/article/details/111590311 ）  
分别是 system bus、virtual bus 和 platform bus。它们并不是一个实际存在的bus（像USB、I2C等），而是为了方便设备模型的抽象，而虚构的。

1. system bus 是旧版内核提出的概念，用于抽象系统设备（如CPU、Timer等等）。而新版内核认为它是个坏点子，因为任何设备都应归属于一个普通的子系统（New subsystems should use plain subsystems, 
drivers/base/bus.c, line 1264），所以就把它抛弃了（不建议再使用，它的存在只为兼容旧有的实现）。
2. virtaul bus 是一个比较新的bus，主要用来抽象那些虚拟设备，所谓的虚拟设备，是指不是真实的硬件设备，而是用软件模拟出来的设备，例如虚拟机中使用的虚拟的网络设备。
3. platform bus 就比较普通，它主要抽象集成在CPU（SOC）中的各种设备。这些设备直接和CPU连接，通过总线寻址和中断的方式，和CPU交互信息。

* bus 的作用
任何 bus 维护两个表，一个是设备链表，一个是驱动链表。bus 负责把设备和驱动进行匹配关联，只有得到正确关联的设备和驱动，才能正常工作。设备和驱动的匹配，其实就是根据 bus 的 match 接口进行，一旦匹配成功就会调用 probe 接口，在probe函数中实现设备的初始化、各种配置以及生成用户空间的文件接口。 probe 函数是总线在匹配成功时调用的函数，bus->probe 和 drv->probe 中只会有一个起效，同时存在时使用 bus->probe

bus 在系统中的体现就是 /sys/bus 目录，/sys 目录是个非常有用的目录，对于 bus, class, device, driver 均有体现，方便维护与理解。

# class 的由来
其实就是根据人们的日常习惯对设备进行分类的一种表现形式，比如：键盘和鼠标我们都看成输入设备，属于输入设备分类。此外还有 net 网络设备分类，block 块设备分类，tty 设备分类。就是符合人们生活习惯的一种分类方式。这些分类相同的设备，具有很多共同特性，非常符合面向对象设计思想，这样就可以设计出基类对象，设备或驱动的编写都可以基于这些基础对象类进一步扩展，大家有了这个天然的约定，理解和维护都非常方便。

class 在系统中的体现就是 /sys/class 目录

# device 设备
这里的 device 就是指的硬件设备，无需过多理解。这里说一下小典故，早期的设备代码都是耦合到内核代码内的，对于嵌入式设备，这些设备的适配给内核带来非常大的问题，内核 OMAP development tree 的维护者，发送了一封邮件给 Linus，请求提交 OMAP 平台代码修改，并附带修改以及如何解决 merge conficts，鉴于此原因还引起了 Linus 的怒吼 “Gaah.Guys, this whole ARM thing is a f*cking pain in the ass.” 对于此问题的解决，就是后来的设备树 devicetree，设备树的优点不言而喻。

* 设备树的优点
1. 实现驱动代码与设备硬件信息相分离。
2. 通过被 bootloader(uboot) 和 Linux 传递到内核， 内核可以从设备树中获取对应的硬件信息。
3. 对于同一 SOC 的不同主板，只需更换设备树文件即可实现不同主板的无差异支持，而无需更换内核文件，实现了内核和不同板级硬件数据的拆分。  
设备树的出现，让内核摆脱由于设备的改变，内核也必须随之进行变动的问题，带来极大的好处（ 具体带来哪些好处，需要读者自己揣摩了 ）

* 设备树的加载  
dts -> dtb -> device_node -> platform_device  
上述流程网上有大量的资料进行解释，经过上述流程，系统启动后，就有了对应的设备节点，有了设备节点，利用驱动就可以驱动这些设备进行工作了。

* x86 硬件设备的加载 ( 对于 x86/64 的硬件加载这篇文章写的比较好，下面的一小段话，间接抄过来了，对此表示感谢。https://www.zhihu.com/question/475730584/answer/2035883968 )  
设备树是因为嵌入式场景而导入的，对于 x86/64 上许多设备是基于 PCI/PCIe 总线的，可以自动枚举，这种设备不需要设备树来告知系统. 当然也有一些设备是无法枚举或自发现的，但这是另一波人维护的内核代码，他们使用的叫平台设备（platform）。x86系统的硬件信息保存在 2 个地方，ACPI 和 SMBIOS 。在x86启动阶段，操作系统会从 BIOS 芯片上的 ACPI 表的读出硬件信息，但是这部分硬件信息相对设备树中的简单，现在的 x86 芯片所有的外部总线都是挂载到 PCI 总线上， PCI 提供了自枚举的功能，操作系统无法枚举时，也可以交由 APCI 枚举. ACPI 数据不能随意更改，而 SMBIOS 数据内容在系统启动时会被更新。一些系统监控，底层功能扩展，性能优化软件更倾向于从SMBIOS中获取所需数据，Linux则提供了 dmidecode 命令可以 dump 出 SMBIOS 的所有数据。包括 ARM，RISCV等其他平台近年同样引入了对 ACPI 和 SMBIOS 的支持。
设备启动 -> 系统固件 ( BIOS 设置、初始化和自检 ) -> 系统固件使用固件初始化期间获得的信息，根据需要使用各种平台配置和电源接口数据更新ACPI表 -> 引导程序 -> 启动内核 -> 内核从 ACPI 表读取设备信息并加载驱动

系统启动过程分析，下面这篇文章不错 https://www.jianshu.com/p/35fb17ad825e

device 在系统中的体现就是 /sys/devices 目录

# driver 驱动
这里的 driver 就是指硬件的驱动程序，通过上述介绍，我们知道 bus 管理 device 和 driver。bus 就像牵线的红娘，驱动 device 和 driver 进行匹对，配对原则很多，主要的是根据驱动的名字和设备的名字进行匹对。匹配方式很多，方法也各有不同，网上有很多相对应的文章对此进行介绍，想深入研究的同学可以搜索一下，进行深入研究。一旦匹配成功，进一步就是调用驱动的 probe 接口进行进一步确认。一旦匹配成功，我们的应用层程序就可以操作对应的设备了。

driver 在系统中的体现就是 /sys/module/ 下的一个子目录， /sys/module 下加载了所有的驱动模块( 包括内核 builtin 的模块 )，总线模块，设备模块 

# 怎么表达上述概念
其实上述的四个概念 bus, class, device, driver 之间的关系相当复杂，一个设备属于某个 bus, 同时按分类又属于某个 class， 按功能设备对接一个或多个驱动；驱动对象也存在类似的关系，为了更好的表达这些关系，需要一些高度抽象的设计，把这些对象的公共部分提取出来，这就是 kobject, kset, ktype 对象的由来，这些结构的抽象对于 linux 设备驱动管理奠定了夯实的基础。

上述的分类概念，就类似一个中年男人，在家里针对不同的家庭成员, 可能是儿子，丈夫，父亲的角色；在公司则是员工，同事，组长，为了更精确的描述这个人的不同角色，才有了儿子，丈夫，父亲，员工，同事，组长的这些词，同理 bus, class 的也是同样的设计理念，为了描述男人这个对象这么复杂的关系图谱，需要很精巧的数据结构配合，这就是 kobject, kset, ktype。 家庭，公司可以用 kset 表示， 男人可以用 kobject, 男人的角色属性（儿子，丈夫，父亲，员工 ... ）用 ktype 进行表示。网上很多文章花了很多 kobject, kset, ktype 的关系图，如果不真正理解它的涵义，很容易被绕晕，其实就是图的表达，这张图由多个树 tree 组成，树的组成则是由同一个对象在由于角色关系形成的。明白这个道理 kobject, kset, ktype 基本上就能理解此结构的最终涵义!

# kobject 对象
从面向对象的思路来看，上述的概念实例化后都可以看成一个对象，对于一个对象就肯定有基本的属性，比如：名字，状态，引用计数等。如果不对基类对象做基本定义，就像盖房子，用的砖五花八门，大小不一样，颜色不一样，底层的不统一，导致上层的结构的不可靠。比如：对于一个设备定义，可能出现如下定义
```C
// 设备定义 1
struct device1 {
    const char         *mingzi;                 // 中文拼音    
};

// 设备定义 2
struct device2 {
    const char         *dev_name;               // 简写
} ;

// 设备定义 3
struct device3 {
    const char         *usb_name;               // 根据分类定义
} ;
```
其实上述结构定义的目的，就是定义一个 name 字段，一个字段就能搞定；由于每个人的理解能力，知识深度，地域文化差异，可能有上千种定义方式；这样的话，所有建立此代码的工程应用都高度耦合这段结构，维护工作量巨大，通用性也极差。由于设备的复杂性，属性字段更多，对于每个字段各种奇葩的定义都有可能发生。这样设备驱动的代码又是各自为战，高耦合的问题还是没有真正解决。要解决此问题，就必须有个基础对象的规范定义，这就是 kobject 对象的由来。下面看看 kobject 的定义，定义如下：
```C
struct kobject {
    const char		    *name;                  // kobject的名字，且作为一个目录的名字
    struct list_head	entry;                  // 连接下一个kobject结构
    struct kobject		*parent;                // 指向父亲kobject结构体，如果父设备存在
    struct kset		    *kset;                  // 指向kset集合
    struct kobj_type	*ktype;                 // 指向kobject的属性描述符
    struct sysfs_dirent	*sd;                    // 对应sysfs的文件目录
    struct kref		    kref;                   // kobject的引用计数
    unsigned int        state_initialized:1;    // 表示该kobject是否初始化
    unsigned int        state_in_sysfs:1;       // 表示是否加入sysfs中
    unsigned int        state_add_uevent_sent:1;
    unsigned int        state_remove_uevent_sent:1;
    unsigned int        uevent_suppress:1;
};
```
针对上述结构，我们看到 kobject 包含了最基础的属性，有链表表达，父节点表达，集合表达，类型表达，引用计数表达。这些公共属性，对于 bus, class, device, driver 的划分提供了基础支撑。

# kset 对象
kset 的根据名字就能猜出它的目的，就是方便把设备驱动进行管理的一个对象模型。kset是包含多个 kobject 的集合；如果需要在 sysfs 的目录中包含多个子目录，那需要将它定义成一个 kset；kset 结构体中包含 struct kobject 字段，可以使用该字段链接到更上一层的结构，用于构建更复杂的拓扑结构，整个 sys 目录其实就是利用 kset 进行组织的。利用这种对象模型，可以表达出不同角度的对象组织方式。这样理解 kset 比较形象，就是对应 /sys 下的一个目录节点。

# ktype 对象
ktype 用于表征 kobject 的类型，指定了删除 kobject时要调用的函数， kobject 结构体中有 struct kref 字段用于对 kobject 进行引用计数，当计数值为 0 时，就会调用 kobj_type 中的 release 函数对 kobject 进行释放，这个就有点类似于C++中的智能指针了, ktype 是 kobject 基本方法的实现，主要是一套属性及文件系统的操作方法

```C
struct kobj_type {
        void (*release)(struct kobject *kobj);
        const struct sysfs_ops *sysfs_ops;      // 文件系统的操作方法
        struct attribute **default_attrs;       // use default_groups instead
        const struct attribute_group **default_groups;
        const struct kobj_ns_type_operations *(*child_ns_type)(struct kobject *kobj);
        const void *(*namespace)(struct kobject *kobj);
        void (*get_ownership)(struct kobject *kobj, kuid_t *uid, kgid_t *gid);
};
```

有关讲解 kobject, kset, ktype 相关的文章相当多，这篇也不错 https://www.eet-china.com/mp/a42768.html
```C
struct bus_type {
        const char                      *name;
        const char                      *dev_name;
        struct device                   *dev_root;
        const struct attribute_group    **bus_groups;
        const struct attribute_group    **dev_groups;
        const struct attribute_group    **drv_groups;

        int (*match)(struct device *dev, struct device_driver *drv);
        int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
        int (*probe)(struct device *dev);
        void (*sync_state)(struct device *dev);
        int (*remove)(struct device *dev);
        void (*shutdown)(struct device *dev);
        int (*online)(struct device *dev);
        int (*offline)(struct device *dev);
        int (*suspend)(struct device *dev, pm_message_t state);
        int (*resume)(struct device *dev);
        int (*num_vf)(struct device *dev);
        int (*dma_configure)(struct device *dev);

        const struct dev_pm_ops         *pm;
        const struct iommu_ops          *iommu_ops;
        struct subsys_private           *p;                   // 这个结构体很重要
        struct lock_class_key           lock_key;
        bool                            need_parent_lock;
};

// ------------------------------------------------------------------------------------
// 驱动核心的私有数据，除了驱动核心能处理它，其余一概不能对此结构进行访问与处理 !!!
// ------------------------------------------------------------------------------------
struct subsys_private {
        struct kset                     subsys;
        struct kset                     *devices_kset;        // 代表bus目录下的 device 的子目录
        struct list_head                interfaces;
        struct mutex                    mutex;
        struct kset                     *drivers_kset;        // 代表bus目录下的 driver 的子目录
        struct klist                    klist_devices;        // 是 bus 的设备链表
        struct klist                    klist_drivers;        // 是 bus 的驱动链表
        struct blocking_notifier_head   bus_notifier;
        unsigned int                    drivers_autoprobe:1;
        struct bus_type                 *bus;
        struct kset                     glue_dirs;
        struct class                    *class;
};

```
上述结构表明了 kset 基本上等同一个 /sys 下的目录节点

```C
struct kset {
        struct list_head                list;
        spinlock_t                      list_lock;
        struct kobject                  kobj;                // kset 结构也包含一个 kobject 对象，可以链接到上层对象，组织出更复杂的拓扑结构
        const struct kset_uevent_ops    *uevent_ops;
} __randomize_layout;
```

我们再看看 class 的定义
```C
struct class {
        const char                      *name;
        struct module                   *owner;
        const struct attribute_group    **class_groups;
        const struct attribute_group    **dev_groups;
        struct kobject                  *dev_kobj;

        int (*dev_uevent)(struct device *dev, struct kobj_uevent_env *env);
        char *(*devnode)(struct device *dev, umode_t *mode);

        void (*class_release)(struct class *class);
        void (*dev_release)(struct device *dev);

        int (*shutdown_pre)(struct device *dev);

        const struct kobj_ns_type_operations *ns_type;
        const void *(*namespace)(struct device *dev);

        void (*get_ownership)(struct device *dev, kuid_t *uid, kgid_t *gid);

        const struct dev_pm_ops         *pm;
        struct subsys_private           *p;
};
```

device 的结构定义
```C
struct device {
        struct kobject                  kobj;
        struct device                   *parent;
        struct device_private           *p;
        const char                      *init_name;             /* initial name of the device */
        const struct device_type        *type;
        struct bus_type                 *bus;                   /* type of bus device is on */
        struct device_driver            *driver;                /* which driver has allocated this device */
        void                            *platform_data;         /* Platform specific data, device core doesn't touch it */
        void                            *driver_data;           /* Driver data, set and get with dev_set_drvdata/dev_get_drvdata */
#ifdef CONFIG_PROVE_LOCKING
        struct mutex                    lockdep_mutex;
#endif
        struct mutex                    mutex;                  /* mutex to synchronize calls to its driver. */
        struct dev_links_info           links;
        struct dev_pm_info              power;
        struct dev_pm_domain            *pm_domain;
#ifdef CONFIG_GENERIC_MSI_IRQ_DOMAIN
        struct irq_domain               *msi_domain;
#endif
#ifdef CONFIG_PINCTRL
        struct dev_pin_info             *pins;
#endif
#ifdef CONFIG_GENERIC_MSI_IRQ
        struct list_head                msi_list;
#endif
        const struct dma_map_ops        *dma_ops;
        u64                             *dma_mask;              /* dma mask (if dma'able device) */
        u64                             coherent_dma_mask;      /* Like dma_mask, but for alloc_coherent mappings as not all hardware 
                                                                supports 64 bit addresses for consistent allocations such descriptors. */
        u64                             bus_dma_limit;          /* upstream dma constraint */
        unsigned long                   dma_pfn_offset;
        struct device_dma_parameters    *dma_parms;
        struct list_head                dma_pools;              /* dma pools (if dma'ble) */
#ifdef CONFIG_DMA_DECLARE_COHERENT
        struct dma_coherent_mem         *dma_mem;               /* internal for coherent mem override */
#endif
#ifdef CONFIG_DMA_CMA
        struct cma                      *cma_area;              /* contiguous memory area for dma allocations */
#endif
        /* arch specific additions */
        struct dev_archdata             archdata;
        struct device_node              *of_node;               /* associated device tree node */
        struct fwnode_handle            *fwnode;                /* firmware device node */
#ifdef CONFIG_NUMA
        int                             numa_node;              /* NUMA node this device is close to */
#endif
        dev_t                           devt;                   /* dev_t, creates the sysfs "dev" */
        u32                             id;                     /* device instance */
        spinlock_t                      devres_lock;
        struct list_head                devres_head;
        struct class                    *class;
        const struct attribute_group    **groups;               /* optional groups */
        void (*release)(struct device *dev);
        struct iommu_group              *iommu_group;
        struct dev_iommu                *iommu;
        bool                            offline_disabled:1;
        bool                            offline:1;
        bool                            of_node_reused:1;
        bool                            state_synced:1;
#if defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_DEVICE) || \
    defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU) || \
    defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU_ALL)
        bool                            dma_coherent:1;
#endif
};
```

driver 的结构定义
```C
struct device_driver {
        const char                      *name;
        struct bus_type                 *bus;

        struct module                   *owner;
        const char                      *mod_name;              /* used for built-in modules */
        bool                            suppress_bind_attrs;    /* disables bind/unbind via sysfs */
        enum probe_type                 probe_type;
        const struct of_device_id       *of_match_table;
        const struct acpi_device_id     *acpi_match_table;

        int (*probe) (struct device *dev);
        void (*sync_state)(struct device *dev);
        int (*remove) (struct device *dev);
        void (*shutdown) (struct device *dev);
        int (*suspend) (struct device *dev, pm_message_t state);
        int (*resume) (struct device *dev);
        const struct attribute_group **groups;
        const struct attribute_group **dev_groups;

        const struct dev_pm_ops         *pm;
        void (*coredump) (struct device *dev);

        struct driver_private           *p;
};

struct driver_private {
        struct kobject kobj;
        struct klist klist_devices;
        struct klist_node knode_bus;
        struct module_kobject *mkobj;
        struct device_driver *driver;
};
```

bus ---> p ( struct subsys_private ) ---> subsys ( struct kset ) ---> kobj  
class ---> p ( struct subsys_private ) ---> subsys ( struct kset ) ---> kobj  
device ---> kobj  
driver ---> p ( struct driver_private ) ---> kobj  

综上所述我们的对象基本信息可以通过 kobject 对象进行表示，对象的基本操作方法和属性可以通过 ktype 对象进行表示，对象的组织（链表，树）可以通过 kset 进行表示，这是整个 linux 设备驱动管理模型的基石，是经验的总结。

# sysfs
"sysfs is a ram-based filesystem initially based on ramfs. It provides a means to export kernel data structures, their attributes, and the linkages between them to userspace.” --- documentation/filesystems/sysfs.txt
sysfs 是基于 ram 运行的一种文件系统，类似于 /proc 目录。在系统内对应的就是 /sys 目录。就是内核为用户空间提供了设备的一些存取方式，提供了操作接口。通俗的说，我们通过 /sys 目录可以非常清晰的看到系统对设备管理的整体布局，同时也可以对设备进行读写操作。关于 /sys 目录，上述也有零星提及，主要就是这些目录，这里就不大篇幅的展开了。 /sys/bus, /sys/module, /sys/devices, /sys/class, /sys/firmware ( acpi/smbios )

系统启动过程中，会利用下述命令，对 sysfs 进行挂载，挂载点就是 /sys 目录，因此 /sys 就是 sysfs 的具体体现。  
```shell
mount -t sysfs sysfs /sys
```

# udev
udev 是 linux kernel 的设备管理器，在最新的内核版本中 kernel_3.10 中 udev 已经代替了 devfs、hotplug 等功能。它能够根据系统中的硬件设备的状态动态更新设备文件，包括设备文件的创建，删除等。
udev是硬件平台无关的，属于 user space 的进程，它脱离驱动层的关联而建立在操作系统之上，基于这种设计实现，可以动态修改及删除 /dev 下的设备文件名称和指向，按照我们的设计目的安排和管理设备文件系统，而完成如此灵活的功能只需要简单地修改 udev的配置文件即可，无需重新启动操作系统，udev 是通过 netlink socket 一种套接字与系统进行通信的，大大简化和解耦了应用层的设备管理工作。关于 udev 是个庞大的知识集，规则，工具，这里只是做一个点引，更深入的学习，需要翻阅更专业的资料。

下面几个工具，如果自己做操作系统是需要了解一下的，方便系统启动后让设备能正常加载，保证应用层的正常运行。
* udevd       —— 作为deamon，记录hotplug事件，然后排队后再发送给udev，避免事件冲突（race conditions）。 
* udevtrigger —— 扫描sysfs文件系统，生成相应的硬件设备hotplug事件。 
* udevsettle  —— 查看udev事件队列，等队列内事件全部处理完毕才退出。 

对于系统启动后 init 脚本，一般都是类似于下面的处理
```shell
mkdir -p /dev/.udev/db 
udevd --daemon 
mkdir -p /dev/.udev/queue 
udevtrigger 
udevsettle 
```

说到 udev 不得不提到 mdev ，mdev 相对于 udev 简单很多，基本功能一样，不过它们的底层运行原理是不一样的。mdev 也是提供了设备管理的功能，具体大家可以查看相关的资料进行深入学习

# uevent 机制
设备的 hotplug 是操作系统运行过程，比较常见的操作，属于基本运行的刚性需求。这就避免不了系统需要把这些信息动态通知到用户层，这就是 uevent 机制。uevent 整个机制相对比较清晰，当有新的 device 产生时，kobjct 产生 uevent 事件，通过两种方式通知到用户层 netlink 和 uevent_helper，在用户空间 linux 提供了两个机制一个是 udev( user device ) 通过 nelink 接收 uevent 产生的事件，另外一种方式是通过 uevent_helper，该方式主要是通过回调方式通知用户层。到这里我们就似乎想到了点什么，就是刚刚提到的 udev 和 mdev。

整个设备添加通知流程如下：（重要）  
```
                                                                               |---> kobject_uevent_net_broadcast ( netlink 方式 ) ---> uevent_net_broadcast_untagged ---> netlink_broadcast

device_register ---> device_add ---> kobject_uevent ---> kobject_uevent_env ---|

                                                                               |---> call_usermodehelper ( uevent_helper 方式 )


```

各位同学可以从网上找一下相关 udev 通知的小例子，结合下面的例子进行配合运行，方便对整套系统的运作原理提供非常大的帮助！


# 小例子

例子分为三部分 bus, device, driver，需要更改 Makefile 里面的路径，如果有必要的话。class 的演示没有加入里面，如果有兴趣的同学可以试一试。网上还有很多有关 udev 的小例子，配合下面的例子，一块运行，对于理解 linux 设备驱动管理模型有很大帮助。如果想查看结果，首先编译这三个模块，其次 insmod my_bus.ko && insmod my_device.ko && insmod my_driver.ko，最后可以对这些节点进行读写，具体可以加入打印信息，利用 dmesg 进行查看
```shell
[root@localhost bus]# tree my_bus/
my_bus/
├── devices
│   └── my_dev -> ../../../devices/my_dev
├── drivers
│   └── my_dev
│       ├── bind
│       ├── my_dev -> ../../../../devices/my_dev
│       ├── rwstate
│       ├── uevent
│       ├── unbind
│       └── version
├── drivers_autoprobe
├── drivers_probe
├── rwstate
├── uevent
└── version

5 directories, 10 files

[root@localhost devices]# tree my_dev/
my_dev/
├── driver -> ../../bus/my_bus/drivers/my_dev
├── power
│   ├── async
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_kids
│   ├── runtime_active_time
│   ├── runtime_enabled
│   ├── runtime_status
│   ├── runtime_suspended_time
│   └── runtime_usage
├── rwstate
├── subsystem -> ../../bus/my_bus
├── uevent
└── version

3 directories, 12 files

[root@localhost drivers]# tree my_dev/       
my_dev/
├── bind
├── my_dev -> ../../../../devices/my_dev
├── rwstate
├── uevent
├── unbind
└── version

1 directory, 5 files
```
其中 rwstate 都是可读写节点，我们可以利用 echo -n "bus write test" > /sys/bus/my_bus/rwstate 进行测试写入，也可以利用 cat /sys/bus/my_bus/rwstate 进行查看；每个对象都有一个只读属性 version，利用命令 cat 也可以查看对象的版本信息

* my_bus 模块  
my_bus.c 源文件
```C
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

//------------------------------------------------------------------------------------------------
static char* my_bus_name = "my_bus";
static char my_bus_rwbuf[128] = "readwrite-default";

static ssize_t bus_attrs_version_show(struct bus_type* bus, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: version 1.0.0\n", my_bus_name);
}

static ssize_t bus_attrs_rw_show(struct bus_type* bus, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: %s\n", my_bus_name, my_bus_rwbuf);
}

static ssize_t bus_attrs_rw_store(struct bus_type* bus, const char* buf, size_t count)
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

static ssize_t device_attrs_version_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: version 1.0.0\n", my_device_name);
}

static ssize_t device_attrs_rw_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: %s\n", my_device_name, my_device_rwbuf);
}
static ssize_t device_attrs_rw_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
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

static ssize_t driver_attrs_version_show(struct device_driver* driver, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: version 1.0.0\n", my_driver_name);
}

static ssize_t driver_attrs_rw_show(struct device_driver* driver, char* buf)
{
    return snprintf(buf, PAGE_SIZE, "%s: %s\n", my_driver_name, my_driver_rwbuf);
}
static ssize_t driver_attrs_rw_store(struct device_driver* driver, const char* buf, size_t count)
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

int my_match(struct device* dev, struct device_driver* drv)
{
    // 比较设备的bus_id与驱动的名字是否匹配，匹配一致则在insmod驱动 时候调用probe函数 
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
```
Makefile 文件
```C
obj-m := my_bus.o

SRC_DIR = $(shell pwd)
KERN_DIR = /lib/modules/$(shell uname -r)/build

modules:
	make -C ${KERN_DIR} M=${SRC_DIR} modules

modules_install:
	make -C ${KERN_DIR} M=${SRC_DIR} modules_install

clean:
	make -C ${KERN_DIR} M=${shell pwd} modules clean
```

* my_device 模块  
my_device.c 源文件
```C
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

extern struct bus_type my_bus_type;

static void my_dev_release(struct device *dev)
{
}

struct device my_dev = {
     .init_name = "my_dev",
     .bus = &my_bus_type,   
     .release = my_dev_release,
};

static int __init my_device_init(void)
{
    int ret;
    ret = device_register(&my_dev);
    return ret;
}

static void __exit my_device_exit(void)
{
    device_unregister(&my_dev);
}

MODULE_AUTHOR("freeabc");
MODULE_DESCRIPTION("my_device demo!");
MODULE_LICENSE("GPL");

module_init(my_device_init);
module_exit(my_device_exit);
```
Makefile 文件
```C
obj-m := my_device.o

SRC_DIR = $(shell pwd)
KERN_DIR = /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS=/root/device_mode/my_bus/Module.symvers

modules:
	make -C ${KERN_DIR} M=${SRC_DIR} modules

modules_install:
	make -C ${KERN_DIR} M=${SRC_DIR} modules_install

clean:
	make -C ${KERN_DIR} M=${shell pwd} modules clean
```

* my_driver 模块  
my_driver.c 源文件
```C
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
    .bus = &my_bus_type,
    .probe = my_probe,
};

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
```
Makefile 文件
```C
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
```