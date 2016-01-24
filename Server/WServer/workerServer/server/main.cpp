/*======================================================
    > File Name: main.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月23日 星期三 13时59分21秒
 =======================================================*/

#include "Server.h"
#include "../info/Timer.h"
#include <signal.h>

/* timer class */
struct event* timer::time_event = NULL;
struct event_base* timer::timebase = NULL;
redisContext* timer::myRedisContext = NULL;

/* server */
struct event_base* WorkerServer::base = event_base_new();
ThreadPool WorkerServer::threadpool(50);
std::mutex WorkerServer::g_lock;
std::map<std::string, int> WorkerServer::unfinished_file;

int main(int argc, char **argv){
    if(argc < 2)
    {
        perror("argument error");
        exit(1);
    }
    /* work ip and port */
    WorkerServer worker(argv[1], atoi(argv[2]));
    /* redis ip */
    timer t("127.0.0.1");
    std::thread tid(&t.run);
    worker.run();
    tid.join();

    return 0;
}
