/*======================================================
    > File Name: main.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月24日 星期日 13时40分22秒
 =======================================================*/


#include "Timer.h"

struct event* timer::time_event = NULL;
struct event_base* timer::timebase = NULL;
redisContext* timer::myRedisContext = NULL;

int main()
{
    timer t("127.0.0.1");
    t.run();
}
