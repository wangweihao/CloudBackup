/*======================================================
    > File Name: string.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月26日 星期六 02时55分43秒
 =======================================================*/

#include <iostream>
#include <string>
#include <stdio.h>

int main()
{
    std::string s = "hello world";
    const char *p = s.c_str();
    printf("%s\n", p);
    char *c = "qwe";
    std::string ss(c);
    std::cout << ss << std::endl;
}
