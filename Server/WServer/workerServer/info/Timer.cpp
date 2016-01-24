/*======================================================
    > File Name: Timer.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月24日 星期日 12时14分25秒
 =======================================================*/


#include "Timer.h"

timer::timer(const char *redisIp, int port)
{
    timeval tv = {SEC, USEC};
    timebase = event_base_new();
    myRedisContext = (redisContext*)redisConnectWithTimeout(redisIp, port, tv);
    if((myRedisContext == NULL) || (myRedisContext->err))
    {
        if(myRedisContext)
        {
            std::cout << "connect error:" << myRedisContext->errstr <<std::endl;
        }else{
            std::cout << "connect error: can't allocate redis context" << std::endl;
        }
    }
    time_event = evtimer_new(timebase, sendInfoToRedis, myRedisContext);
    if(evtimer_add(time_event, &tv) < 0)
    {
        perror("time event add error");
        exit(1);
    }
}

timer::~timer() {}

void 
timer::run()
{
    event_base_dispatch(timebase);
}

bool 
timer::addTimeEvent(struct event* time_event, struct timeval tv)
{
    if(evtimer_add(time_event, &tv) < 0)
    {
        return true;
    }else{
        return false;
    }
}

bool
timer::delTimeEvent(struct event* time_event)
{
    if(evtimer_del(time_event) < 0)
    {
        return true;
    }else{
        return false;
    }
}


void 
timer::sendInfoToRedis(evutil_socket_t fd, short event, void* arg)
{
    timeval tv = {SEC, USEC};
    int diskInfo = getDiskStat();
    int netInfo  = getNetStat();

    redisReply *reply = (redisReply*)redisCommand(myRedisContext, "INFO");
    std::cout << reply->str << std::endl;

    /* 进行下一次注册 */
    evtimer_add(time_event, &tv);
}

int 
timer::getDiskStat()
{
    const char* diskstat = "/proc/diskstats";
    std::string line;
    std::ifstream reader(diskstat);
    
    while(!reader.eof())
    {
        std::getline(reader, line);
        if(reader.fail())
        {
            break;
        }
        std::cout << line << std::endl;
    }

    /* 假设计算出磁盘 IO 比例为 70% */
    return 70;
}

int 
timer::getNetStat()
{
    const char *netstat = "/proc/net/dev";
    std::string line;
    std::ifstream reader(netstat);

    while(!reader.eof())
    {
        std::getline(reader, line);
        if(reader.fail())
        {
            break;
        }
        std::cout << line << std::endl;
    }

    return 70;
}

