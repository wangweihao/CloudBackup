/*======================================================
    > File Name: Server.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月24日 星期四 13时28分54秒
 =======================================================*/

#include "Server.h"


WorkerServer::
WorkerServer(std::string w_ip, int w_port, std::string b_ip, int b_port):ip(w_ip), port(w_port)
{
    if(!base)
    {
        perror("init event_base error");
        exit(1);
    }

    /* init server socket */
    bzero(&server, sizeof(server));    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);
    std::cout << "ip:" << ip << " port:" << port << std::endl;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0)
    {
        perror("socket error");
        exit(1);
    }

    /* set nonblocking */
    evutil_make_socket_nonblocking(listen_fd);
    
    if(bind(listen_fd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        perror("bind error");
        exit(1);
    }
    
    if(listen(listen_fd, MAX_BACKLOG) < 0)
    {
        perror("listen error");
        exit(1);
    }

    /* new accept event */
    accept_event = event_new(base, listen_fd, EV_READ | EV_PERSIST, accept_callback, (void*)base);

    /* register accept event */
    event_add(accept_event, NULL);
}

void 
WorkerServer::run()
{
    std::cout << "loop" << std::endl;
    /* start event loop */
    event_base_dispatch(base);
}

WorkerServer::~WorkerServer()
{

}


/* 上传文件
 * 考虑两个用户同时上传一个文件怎么办，秒传？万一一个传到一半断了不传怎么办
 * 考虑断点续传问题
 * */
bool 
WorkerServer::handler_upload(int socket, std::string md5, long size, long offset, int threadNum)
{
    int filefd;
    long file_size = 0;
    long accumu_size = 0;
    int pipefd[2];
    ssize_t ret = 0;

    /* file size */
    file_size = size - offset;

    /* open file*/
    if((filefd = open(md5.c_str(), O_WRONLY | O_CREAT)) < 0)
    {
        perror("open file error");
        return false;
    }
    /* offset not equal zero, breakpoint upload */
    if(offset > 0)
    {
        if(lseek(filefd, offset, SEEK_CUR) < 0)
        {
            perror("lseek file error");
            return false;
        }
    }

    if(pipe(pipefd) < 0)
    {
        perror("create pipe error");
        return false;
    }
    /* upload file */
    while(1)
    {
        if((ret = splice(socket, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1){
            perror("socket ==> pipefd[1] splice error");
            break;
        }else if(ret == 0)
        {
            break;
        }
        if((ret = splice(pipefd[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1){
            perror("pipe[0] ==> file splice error");
            break;
        }
        accumu_size += ret;
    }

    /* lots of user operating unfinished_file */
    std::lock_guard<std::mutex> locker(g_lock);

    if(accumu_size == file_size)
    {
        std::cout << "upload file success" << std::endl;
        /* file transfer success, delete unfinished file*/
        auto iter = unfinished_file.find(md5);
        if(iter != unfinished_file.end())
        {
            unfinished_file.erase(iter);             
        }
    }else{
        std::cout << "upload file error" << std::endl;
        /* 断点续传，用户未上传成功，设置定时事件，
         * 若用户在给定时间内没有断点续传，则触发定时事件，
         * 删除服务器本地的文件 */

        /* 先考虑某一时刻只有一个用户上传此文件 */
        unfinished_file.insert(std::make_pair(md5, 1));

        /* 设置定时事件 */
        struct timeval tv = {300, 0};
        struct event* timeout_event = evtimer_new(base, timeout_callback, (void*)md5.c_str());
        event_add(timeout_event, &tv);

        return false;
    }

    return true;
}

bool
WorkerServer::handler_download(int socket, std::string md5, long size, long offset, int threadNum)
{
    int filefd;
    long file_size = 0;
    long accumu_size = 0;
    ssize_t ret = 0;
    struct stat file_state;

    /* open file */
    if((filefd = open(md5.c_str(), O_RDONLY)) < 0)
    {
        perror("file open error");
        return false;
    }

    /* if file size equal zero, obtain file size */
    if(size == 0)
    {
        if(stat(md5.c_str(), &file_state))
        {
            perror("obtain file size error");
            return false;
        }
        size = file_state.st_size;   
    }
    
    /* file size */
    file_size = size - offset;

    /* offset not equal zero, breakpoint download */
    if(offset > 0)
    {
        if(lseek(filefd, offset, SEEK_CUR) < 0)
        {
            perror("lseek file error");
            return false;
        }
    }

    if((ret = sendfile(socket, filefd, &offset, file_size)) < 0)
    {
        perror("sendfile error");
        return false;
    }

    if(ret == file_size)
    {
        std::cout << "sendfile success" << std::endl;
    }else{
        std::cout << "sendfile error" << std::endl;
        return false;
    }

    return true;
}


/* 多线程下载
 * 将文件均分成多个任务，
 * 加入到任务队列中，
 * 由多个线程处理*/
bool 
WorkerServer::multi_thread_download(int socket, std::string md5, long size, long offset, int threadNum)
{
    struct stat file_state;
    
    /* if file size equal zero, obtain file size */
    if(size == 0)
    {
        if(stat(md5.c_str(), &file_state))
        {
            perror("obtain file size error");
            return false;
        }
        size = file_state.st_size;   
    }

    /* detect thread num */
    long one_size = size/threadNum;
    
    /* block download */
    for(int i = 0; i < threadNum-1; ++i)
    {
        Handler handler = std::make_tuple(WorkerServer::file_block_download, socket, md5, (one_size*i), (one_size*(i+1)), 0);
        threadpool.AddTask(std::move(handler));
        
    }
    /* 最后一块文件大小需要特殊处理 */
    Handler handler = std::make_tuple(WorkerServer::file_block_download, socket, md5, (one_size*(threadNum-1)), size, 0);
    threadpool.AddTask(std::move(handler));

    return true;
}

bool 
WorkerServer::file_block_download(int socket, std::string md5, long start, long end, int flag)
{
    int filefd;
    long block_size = end - start;
    ssize_t ret;
    
    /* open file */
    if((filefd = open(md5.c_str(), O_RDONLY)) < 0)
    {
        perror("file open error");
        return false;
    }


    if((ret = sendfile(socket, filefd, &start, block_size)) < 0)
    {
        perror("sendfile error");
        return false;
    }

    if(ret == block_size)
    {
        std::cout << "sendfile success" << std::endl;
    }else{
        std::cout << "sendfile error" << std::endl;
        return false;
    }
}

void
WorkerServer::accept_callback(evutil_socket_t listen_fd, short event, void* arg)
{
    std::cout << "收到连接" << std::endl;

    struct event *read_event;
    struct event *error_event;
    struct sockaddr_storage client;
    socklen_t length = sizeof(client);
    int fd = accept(listen_fd, (struct sockaddr*)&client, &length);
    if(fd < 0)
    {
        perror("accept error");
        return;
    }else{
        struct bufferevent *bev;   
        evutil_make_socket_nonblocking(fd);
        read_event  = event_new(base, fd, EV_READ  | EV_PERSIST, read_callback,  (void*)base);
        event_add(read_event, 0);
    }
}

void 
WorkerServer::read_callback(evutil_socket_t socket, short event, void *arg)
{
    std::cout << "read callback" << std::endl;
    /* recv json and return ack then download */
    char *buffer;
    char jsonMsgLen[2];
    Json::Reader reader;
    Json::Value  value;

    /* mark is a flag
     * mark = 1 upload    (breakpoint_upload)
     * mark = 2 download  (breakpoint_download)
     * mark = 3 multithread download
     * */
    int mark = 0;
    std::string md5;
    unsigned long size = 0;
    unsigned long offset = 0;
    int threadNum = 0;



    /* recv request (json)*/
    /*
     * Json:
     * {
     *      mark:1,     标识
     *      md5:asd7qwebuuaweg12i3ehiqwueh, 
     *      size:1900,  文件大小
     *      offset:0,   偏移量  偏移量不为 0 则进行断点上传或下载
     *      thread:1,   多线程下载
     * }
     * */
    /* 先接收 2 字节 json 长度，接着接收 json 串 */
    recv(socket, jsonMsgLen, 2, 0);
    //short bufferLength = parse(jsonMsgLen);
    short bufferlength = 10;
        
    buffer = (char*)malloc(sizeof(char) * bufferlength);
    /* 即使设置 MSG_WAITALL 标识，不代表能接收 bufferlength 长度的数据包，需自己判定 */
    if(recv(socket, buffer, bufferlength, MSG_WAITALL) == bufferlength)
    {
        printf("%s\n", buffer);
    }else{
        return;
    }

    /* parse json */
    if(reader.parse(buffer, value))
    {
        if(!value["mark"].isNull())
        {
            mark = value["mark"].asInt();
            md5 = value["md5"].asString();
            size = value["size"].asLargestUInt();
            offset = value["offset"].asLargestUInt();
            threadNum = value["threadNum"].asInt();

            if(detect_paramenter_correct(mark, md5, size, offset, threadNum) == false){
                perror("detect parament correct error");
                return;
            }

            /* create handle event */
            Handler handle_event;

            /* parse mark, handler event */
            switch(mark){
                case 1:
                    handle_event = std::make_tuple(WorkerServer::handler_upload, socket, md5, size, offset, threadNum);
                    threadpool.AddTask(std::move(handle_event));
                    break;
                case 2:
                    handle_event = std::make_tuple(WorkerServer::handler_download, socket, md5, size, offset, threadNum);
                    threadpool.AddTask(std::move(handle_event));
                    break;
                case 3:
                    multi_thread_download(socket, md5, size, offset, threadNum);
                    handle_event = std::make_tuple(WorkerServer::multi_thread_download, socket, md5, size, offset, threadNum);
                    threadpool.AddTask(std::move(handle_event));
                    break;
                default:
                    break;
            }
        }
    }
}

void 
WorkerServer::error_callback(evutil_socket_t socket, short event, void *arg)
{
    std::cout << "error_callaback" << std::endl;
    sleep(1);
}


/* 定时器回调函数
 * 若用户上传文件中断开连接
 * 服务器则设定定时保存该文件
 * 若用户长时间未断点续传
 * 超时删除未完成上传的文件*/
void
WorkerServer::timeout_callback(evutil_socket_t socket, short event, void* arg)
{
    std::map<std::string, int>::iterator iter;
    std::string md5((char*)arg);
    /* 尽快释放锁，减小粒度 */
    {
        std::lock_guard<std::mutex> locker(g_lock);
        iter = unfinished_file.find(md5);
    }
    if(iter == unfinished_file.end())
    {
        return;
    }else{
        unfinished_file.erase(iter);
        /* 用户超时，删除未完成上传的文件 */
        remove(md5.c_str());
    }
}

bool detect_paramenter_correct(int mark, std::string md5, unsigned long size, unsigned long offset, int threadNum)
{
    if((mark > 0 && mark < 4) && (!md5.empty()) && (size >= 0) && (offset >=0) && (threadNum >= 0))
    {
        return true;
    }else{
        return false;
    }
}

