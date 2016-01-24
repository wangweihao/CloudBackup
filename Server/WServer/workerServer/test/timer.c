/*======================================================
    > File Name: timer.c
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月23日 星期六 21时54分42秒
 =======================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <event2/event.h>
#include <event2/event_struct.h>

struct self_tv{
    struct event* timeout;
    struct timeval tv;
    int order;
};

void sigroute(int fd, short event, void*arg)
{
    struct self_tv *st = (struct self_tv*)arg;
    printf("%d wake up\n", st->order);
    st->tv.tv_sec = st->tv.tv_sec +1;
    evtimer_add(st->timeout, &(st->tv));
}


int main()
{
    struct event_base *eb;
    struct self_tv *st;
    int i = 1;
    eb = event_base_new();
    for(int i = 0; i <= 1000; ++i)
    {
        st = (struct self_tv*)malloc(sizeof(struct self_tv));
        st->tv.tv_sec = 5;
        st->tv.tv_usec = 0;
        st->order = i;
        st->timeout = evtimer_new(eb, sigroute, st);
        evtimer_add(st->timeout, &st->tv);

    }
    event_base_dispatch(eb);
    return 0;
}
