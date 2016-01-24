/*======================================================
    > File Name: main.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月24日 星期日 13时40分22秒
 =======================================================*/


#include "Timer.h"

int main()
{
    timer t("127.0.0.1");
    struct event* timeout;
    struct timeval tv = {3, 0};
    timeout = evtimer_new(t.timebase, sendInfoToRedis, t.myRedisContext);
    t.addTimeEvent(timeout, tv);
    t.run();
}
