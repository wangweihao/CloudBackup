/*======================================================
    > File Name: 1.c
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月26日 星期六 12时24分12秒
 =======================================================*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>


int main()
{
    char *p = "hello world\n";
    char *p2 = "hello world\n";
    int fd = open("1", O_WRONLY);
    write(fd, p, strlen(p));
    write(fd, p2, strlen(p2));
}
