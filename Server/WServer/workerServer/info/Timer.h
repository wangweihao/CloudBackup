/*======================================================
    > File Name: Timer.h
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月24日 星期日 12时07分37秒
 =======================================================*/


#ifndef _TIMER_H_
#define _TIMER_H_

/* c++ header */
#include <string>
#include <iostream>
#include <fstream>

/* c header */
#include <signal.h>
#include <sys/time.h>

/* libevent header */
#include <event2/event.h>
#include <event2/event_struct.h>

/* redis header */
#include <hiredis/hiredis.h>

#define SEC 3
#define USEC 0

class timer{
    public:
        timer(const char *redisIp, int port = 6379);
        ~timer();

        static void run();
        static bool addTimeEvent(struct event* time_event, struct timeval tv);
        static bool delTimeEvent(struct event* time_event);
        
        /* obtain disk and net info */
        static void sendInfoToRedis(evutil_socket_t fd, short event, void* arg);
        static int getDiskStat();
        static int getNetStat();

    private:
        /* redis */
        static redisContext *myRedisContext;
        
        /* libevent loop */
        static struct event *time_event;
        static struct event_base *timebase;
};



#endif
