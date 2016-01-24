/*======================================================
    > File Name: testRedis.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月24日 星期日 13时16分16秒
 =======================================================*/


#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main()
{
    struct timeval timeout = {2, 0};
    redisContext *pRedisContext = (redisContext*)redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if((pRedisContext == NULL) || (pRedisContext->err))
    {
        if(pRedisContext)
        {
            std::cout << "connect error:" << pRedisContext->errstr << std::endl; 
        }else{
            std::cout << "connect error: can't allocate redis context" << std::endl;
        }
        return -1;
    }

    redisReply *pRedisReply = (redisReply*)redisCommand(pRedisContext, "INFO");
    std::cout << pRedisReply->str << std::endl;

    freeReplyObject(pRedisReply);
    
    return 0;
}
