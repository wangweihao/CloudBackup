/*======================================================
    > File Name: Server.h
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2015年12月23日 星期三 13时57分56秒
 =======================================================*/

#ifndef _SERVER_H_
#define _SERVER_H_


/* C++ header */
#include <iostream>
#include <string>
#include <map>
#include <mutex>

/* C   header*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <json/json.h>


/* libevent header*/
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

/* my head */
#include "ThreadPool.h"


#define MAX_BACKLOG      1024
#define JSONMSG_BUF      256
#define LINK_BANDWIDTH   100   //100MBps
#define RTT              50    //50ns                                  


class WorkerServer{
    public:
        /* init function */
        WorkerServer(std::string w_ip, int w_port);

        /* free resourse */
        ~WorkerServer();

    public:
        void run();

    public:
        /* file upload    (breakpoint upload)*/
        static bool handler_upload(int socket, std::string md5, long size, long offset, int threadNum);
        /* file download  (breakpoint download)*/
        static bool handler_download(int socket, std::string md5, long size, long offset, int threadNum);
        /* file block download (multi thread download)*/
        static bool file_block_download(int socket, std::string md5, long start, long end, int threadNum);

    private:
        /* event callback */
        static void error_callback(evutil_socket_t socket, short event, void* arg);
        static void read_callback(evutil_socket_t socket, short event, void* arg);
        static void accept_callback(evutil_socket_t listen_fd, short event, void* arg);
        static void timeout_callback(evutil_socket_t socket, short event, void* arg);

        /* alloc fd struct */
        static struct fd_state* alloc_fd_state(struct event_base *base, evutil_socket_t fd);
        static void free_fd_state(struct fd_state* state);

        /* set tcp paramenter */
        static bool set_tcp_buf(evutil_socket_t socket, int sock_buf_size);

    private:    
        /* WorkerServer ip and port */
        std::string ip;
        int port;

        /* server info*/
        struct sockaddr_in server;

        /* listen socket */
        evutil_socket_t listen_fd;
        
        /* libevent accept event */
        struct event *accept_event;

        static struct event_base *base;

        static std::mutex g_lock;

    private:
        /* help */
        static ThreadPool threadpool;
        /* save unfinished file transfer */
        static std::map<std::string, int> unfinished_file;
};

union Len{
    char c[2];
    short length;
};

bool detect_paramenter_correct(int, std::string, unsigned long, unsigned long, int);

#endif
