/*======================================================
    > File Name: 2.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月26日 星期六 12时32分35秒
 =======================================================*/

#include<iostream>
#include <vector>

int main()
{
    std::vector<char> ivec;
    ivec.resize(1024);
    std::cout << ivec.size() << std::endl;
    std::cout << ivec.capacity() << std::endl;
}
