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

class timer{
    public:
        timer(const char *redisIp, int port = 6379);
        ~timer();

        void run();
        bool addTimeEvent(struct event* time_event, struct timeval tv);
        bool delTimeEvent(struct event* time_event);

    public:
        /* redis */
        struct timeval timeout = {2, 0};
        redisContext *myRedisContext;
        /* libevent loop */
        struct event_base *timebase;
};


void sendInfoToRedis(evutil_socket_t fd, short event, void* arg);
int getDiskStat();
int getNetStat();

#endif
