#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

int main()
{
    int fd, ret;
    fd_set rds;
    char buffer[128];

    char *name = "/dev/mydrv0";
#if 0
    char *name = "/dev/mydrv1";
#endif
    
    // 初始化 buffer
    strcpy(buffer, "read inital!");
    printf("buffer : %s %s\n", name, buffer);
    
    // 打开设备文件 
    fd = open(name, O_RDWR);
    if ( fd < 0 ) {
        printf("open device %s failed.\n", name);
        return -1;
    }
    
    FD_ZERO(&rds);
    FD_SET(fd, &rds);

    // 清除 buffer 
    strcpy(buffer,"buffer is null!");
    printf("now buffer is : %s\n\n", buffer);
    
    ret = select(fd + 1, &rds, NULL, NULL, NULL);
    if ( ret < 0 ) {
        printf("select device %s error!\n", name);
        return -1; 
    }

    if ( FD_ISSET(fd, &rds) ) {
        printf("read device %s data... ...\n", name);
        read(fd, buffer, sizeof(buffer)); 
    }
    
    // 检测结果
    printf("read buffer : %s\n", buffer);
    close(fd);
    
    return 0;    
}
