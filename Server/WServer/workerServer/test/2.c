/*======================================================
    > File Name: 2.c
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月26日 星期六 01时53分45秒
 =======================================================*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


int main()
{
    int fd = open("1", O_RDONLY);
    lseek(fd, 1, SEEK_CUR);
    //lseek(fd, SEEK_SET, 0);
    char buf[1024];
    read(fd, buf, 1024);
    printf("%s\n", buf);

}
