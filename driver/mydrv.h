#ifndef _MYIOCTL_H_
#define _MYIOCTL_H_

#include <linux/ioctl.h>

struct myioctl_data
{
    unsigned int i_data;
    char s_data[128];
};

#define MYIOCTL_MAGIC '~'
#define MYIOCTL_NONE    _IO(MYIOCTL_MAGIC, 0)
#define MYIOCTL_SET_INT _IOW(MYIOCTL_MAGIC, 1, unsigned int)
#define MYIOCTL_GET_INT _IOR(MYIOCTL_MAGIC, 2, unsigned int)
#define MYIOCTL_SET_STR _IOW(MYIOCTL_MAGIC, 3, char[256])
#define MYIOCTL_GET_STR _IOR(MYIOCTL_MAGIC, 4, char[256])
#define MYIOCTL_SET_OBJ _IOW(MYIOCTL_MAGIC, 5, struct myioctl_data)
#define MYIOCTL_GET_OBJ _IOR(MYIOCTL_MAGIC, 6, struct myioctl_data)

#endif
