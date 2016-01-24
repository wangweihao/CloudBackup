/*======================================================
    > File Name: union.c
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月23日 星期六 16时00分51秒
 =======================================================*/

#include<stdio.h>

union foo{
    char a[2];
    short t;
}a;


int main(){
    a.a[0] = '?';
    a.a[1] = '\0';
    printf("%d\n", a.t);   
}
