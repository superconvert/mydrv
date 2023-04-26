#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "../driver/mydrv.h"

int main()
{
    int fd = -1;
    char buffer[128] = {0};
    char *name = "/dev/mydrv0";
#if 0
    char *name = "/dev/mydrv1";
#endif
    
    // 打开设备文件
    fd = open(name, O_RDWR);
    if ( fd < 0 ) {
        printf("open device %s error!\n", name);
        return -1;
    }

    printf("--------------------------------\n");
    // ioctl 使用
    unsigned int value1 = 2000;
    ioctl(fd, MYIOCTL_SET_INT, &value1);
    printf("set int value : %d\n", value1);

    unsigned int value2 = 0;
    ioctl(fd, MYIOCTL_GET_INT, &value2);
    printf("get int value : %d\n\n", value2);

    printf("--------------------------------\n");
    char * str = "hello mydrv string";
    ioctl(fd, MYIOCTL_SET_STR, str);
    printf("set str value : %s\n", str);

    char buff[256] = {0};
    ioctl(fd, MYIOCTL_GET_STR, buff);
    printf("get str value : %s\n\n", buff);


    printf("--------------------------------\n");
    struct myioctl_data ioctl_data;
    ioctl_data.i_data = 10000;
    strcpy(ioctl_data.s_data, "hello mydrv object");
    ioctl(fd, MYIOCTL_SET_OBJ, &ioctl_data);
    printf("set obj value : %d - %s\n", ioctl_data.i_data, ioctl_data.s_data);

    ioctl(fd, MYIOCTL_GET_OBJ, &ioctl_data);
    printf("get obj value : %d - %s\n\n", ioctl_data.i_data, ioctl_data.s_data);

    
    // 写入设备
    printf("--------------------------------\n");
    sprintf(buffer, "write %s is char dev!", name);
    printf("write device : %s\n", buffer);
    write(fd, buffer, sizeof(buffer));
    
    sleep(5);
    close(fd);
    
    return 0;    

}
