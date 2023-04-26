#include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <linux/fb.h>
 #include <sys/mman.h>
 #include <sys/ioctl.h> 
 
 #define PAGE_SIZE 4096
 
 int main(int argc , char *argv[])
 {
     int i;
     int fd = -1;
     unsigned char *p_map;
    char *name = "/dev/mydrv0";
#if 0
    char *name = "/dev/mydrv1";
#endif
     
     // 打开设备
     fd = open(name, O_RDWR);
     if ( fd < 0 ) {
         printf("open device %s failed.\n", name);
         return -1;
     }
 
     // 内存映射
     p_map = (unsigned char *)mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
     if ( p_map == MAP_FAILED ) {
         printf("mmap device %s failed\n", name);
         return -1;
     }
 
     // 驱动层会写入 "hello mmap"，这里打印一下
     printf("%s", (char *)p_map);
     printf("\n");
 
     munmap(p_map, PAGE_SIZE);
     close(fd);

     return 0;
 }
