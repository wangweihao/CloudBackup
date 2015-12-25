/*======================================================
    > File Name: main.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月23日 星期三 13时59分21秒
 =======================================================*/

#include "Server.h"

struct event_base* WorkerServer::base = event_base_new();
ThreadPool WorkerServer::threadpool(50);

int main(int argc, char **argv){
    if(argc < 4)
    {
        perror("argument error");
        exit(1);
    }
    WorkerServer worker(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]));
    worker.run();

    return 0;
}
