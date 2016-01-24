/*======================================================
    > File Name: Server.cpp
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月24日 星期四 13时28分54秒
 =======================================================*/

#include "Server.h"


struct fd_state{
    int fd;
    short length;

    struct event *read_event;
    struct event *write_event;
};

struct fd_state*
WorkerServer::
alloc_fd_state(struct event_base *base, evutil_socket_t fd)
{
    struct fd_state *state = (struct fd_state*)malloc(sizeof(struct fd_state));
    if(state == NULL)
    {
        return NULL;
    }
    state->read_event = event_new(base, fd, EV_READ | EV_PERSIST, read_callback, state);
    if(state->read_event == NULL)
    {
        free(state);
    }
    return state;
}

void
WorkerServer::
free_fd_state(struct fd_state* state)
{
    event_free(state->read_event);
    free(state);
}


WorkerServer::
WorkerServer(std::string w_ip, int w_port):ip(w_ip), port(w_port)
{
    int bufSize = 0;

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

    /* 设置接收和发送缓冲区大小
     * 计算 BDP(Bandwith Delay Product) 大小
     * BDP = link_bandwidth * RTT
     * bufSize = BDP / threadSize*/
    bufSize = (LINK_BANDWIDTH*RTT)/threadpool.getSize();

    if(set_tcp_buf(listen_fd, bufSize) == false){
        perror("set tcp buffer error");
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

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

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
    close(listen_fd);
}


/* 上传文件
 * 考虑两个用户同时上传一个文件怎么办，秒传？万一一个传到一半断了不传怎么办
 * 考虑断点续传问题
 * */
bool 
WorkerServer::handler_upload(int socket, std::string md5, long size, long offset, int threadNum)
{
    std::cout << "handler_upload" << std::endl;
    
    int filefd;
    long file_size = 0;
    long accumu_size = 0;
    int pipefd[2];
    ssize_t ret = 0;

    md5 += "new";

    /* file size */
    file_size = size - offset;

    /* open file*/
    if((filefd = open(md5.c_str(), O_WRONLY | O_CREAT, 00777)) < 0)
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
    
    while(1)
    {
        if((ret = splice(socket, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1){
            printf("errno:%d\n", errno);
            if(errno == EAGAIN)
            {
                break;
            }
            perror("socket ==> pipefd[1] splice error");
            break;
        }else if(ret == 0)
        {
            break;
        }
        if((ret = splice(pipefd[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1){
            printf("errno:%d\n", errno);
            if(errno == EAGAIN)
            {
                break;
            }
            perror("pipe[0] ==> file splice error");
            break;
        }
        accumu_size += ret;
    }

    std::cout << "accumu_size:" << accumu_size << std::endl; 
    std::cout << "file_size:" << file_size << std::endl; 

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
       * 删除服务器本地该用户残留的文件 */

      /* 先考虑某一时刻只有一个用户上传此文件 */
      unfinished_file.insert(std::make_pair(md5, 1));

      /* 设置定时事件 */
      struct timeval tv;
      tv.tv_sec = 5;
      tv.tv_usec = 0;
      struct event* timeout_event = evtimer_new(base, timeout_callback, (void*)md5.c_str());
      int t = evtimer_add(timeout_event, &tv);

      return false;
    }

    return true;
}

bool
WorkerServer::handler_download(int socket, std::string md5, long size, long offset, int threadNum)
{
    std::cout << "handler_download" << std::endl;
 
    int filefd;
    long file_size = 0;
    long accumu_size = 0;
    ssize_t ret = 0;
    struct stat file_state;

    /* open file */
    if((filefd = open(md5.c_str(), O_RDONLY, 777)) < 0)
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

    std::cout << file_size << std::endl;
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
        
        /*
         * 过早关闭 socket 
         * 当客户端意外结束时，退出事件（触发 EPOLL_IN）无法处理
         * 导致下一个用户（socket 值和上一个相同）复用上一个用户
         * socket，结果两次连接触发一次事件。
         * close(socket);
         * */
    }else{
        std::cout << "sendfile error and ret:" << ret << std::endl;
        return false;
    }

    return true;
}


/* 多线程下载
 * 将文件均分成多个任务，
 * 加入到任务队列中，
 * 由多个线程处理多个 socket
 * 注意：若线程池线程数满足 threadNum 则 threadNum 个线程下载
 * 否则，使用剩余线程数进行下载 */
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
    std::cout << "accept client connect" << std::endl;

    struct event *read_event;
    struct event *error_event;
    struct sockaddr_storage client;
    socklen_t length = sizeof(client);
    int fd = accept(listen_fd, (struct sockaddr*)&client, &length);
    if(fd < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            std::cout << "errno" << std::endl;
            close(fd);
            return;
        }
        perror("accept error");
        return;
    }else{
        struct fd_state *state;
        evutil_make_socket_nonblocking(fd);
        state = alloc_fd_state(base, fd);   
        event_add(state->read_event, 0);
    }
}

void 
WorkerServer::read_callback(evutil_socket_t socket, short event, void *arg)
{
    std::cout << "read callback" << std::endl;
    
    struct fd_state* state = (struct fd_state*)(arg);
    
    /* recv json and return ack then download */
    char *buffer;
    char jsonMsgLen[2];
    Json::Reader reader;
    Json::Value  value;

    /* mark is a flag
     * mark = 1 upload    (breakpoint_upload)
     * mark = 2 download  (breakpoint_download)
     * mark = 3 multithread download (file_block_download)
     * */
    int mark = 0;
    std::string md5;
    unsigned long size = 0;
    unsigned long offset = 0;
    unsigned long start = 0;
    unsigned long end = 0;
    int threadNum = 0;
    
    union Len len;


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
    /* 先接收 2 字节 json 长度 len，接着接受长度为 len 的 json 串 */
    int ret = recv(socket, jsonMsgLen, 2, MSG_WAITALL);
    if(ret < 0)
    {
        if(errno == EAGAIN && errno == EWOULDBLOCK)
        {
            std::cout << "recv errno" << std::endl;
            free_fd_state(state);
            close(socket);
            return;
        }
    }else if(ret == 0)
    {
        free_fd_state(state);
        return;
    }else{
        len.length = 0;
        len.c[0] = jsonMsgLen[0];
        len.c[1] = jsonMsgLen[1];
        printf("bufferlength:%d\n", len.length);
    }
    if((buffer = (char*)malloc(sizeof(char) * len.length)) == NULL)
    {
        perror("malloc error");
        return;
    }
    /* 即使设置 MSG_WAITALL 标识，不代表能接收 bufferlength 长度的数据包，需自己判定 */
    int recvnum;
    if((recvnum = recv(socket, buffer, len.length, MSG_WAITALL)) == len.length)
    {
        printf("buffer:%s\n", buffer);
        event_del(state->read_event);
    }else{
        free_fd_state(state);
        printf("recv return:%d\n", len.length);
        return;
    }


    /* parse json */
    if(reader.parse(buffer, value))
    {
        if(!value["mark"].isNull())
        {
            mark = value["mark"].asInt();
            md5 = value["md5"].asString();
            threadNum = value["threadNum"].asInt();
            if(mark == 3){
                start = value["start"].asLargestUInt();
                end = value["end"].asLargestUInt();
                std::cout << "start:" << start << std::endl;
                std::cout << "end:" << end << std::endl; 
            }else{
                size = value["size"].asLargestUInt();
                offset = value["offset"].asLargestUInt();
                /* check argument is correct*/
                if(detect_paramenter_correct(mark, md5, size, offset, threadNum) == false){
                    perror("detect parament error");
                    return;
                }
                std::cout << "size:" << size << std::endl;
                std::cout << "offset:" << offset << std::endl;
            }

            std::cout << "mark:" << mark << std::endl;
            std::cout << "md5:" << md5 << std::endl;
            std::cout << "threadNum:" << threadNum << std::endl;
  
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
                    handle_event = std::make_tuple(WorkerServer::file_block_download, socket, md5, start, end, threadNum);
                    threadpool.AddTask(std::move(handle_event));
                    break;
                default:
                  break;
            }
        }
    }
    if(buffer != NULL)
    {
        free(buffer);   
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
    std::cout << "timeout" << std::endl;
  
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
        /* 用户操作超时，删除未完成上传的文件 */
        std::cout << "timeout delete file" << std::endl;
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

bool 
WorkerServer::set_tcp_buf(evutil_socket_t socket, int sock_buf_size)
{
    int ret = 0;
    if((ret = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size, sizeof(sock_buf_size))) == -1){
        perror("setsockopt send buffer error");
        return false;
    }
    if((ret = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size, sizeof(sock_buf_size))) == -1){
        perror("setsockopt recv buffer error");
        return false;
    }   
    return true;
}
