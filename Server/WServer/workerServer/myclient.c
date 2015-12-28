#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAXDATASIZE 100

int main(int argc, char *argv[])
{
    int sockfd;                 /* 套接字 */
    int num;                    /* 接受发送数据的大小 */
    char buf[MAXDATASIZE];      /* 缓冲区 */
    struct sockaddr_in server;  /* 存储ip和端口的结构体 */
    
    char* ip = argv[1];
    int port = atoi(argv[2]);

    if (argc != 3)
    {
        printf("argument error, please input ip and port\n");
        exit(1);
    }
        
    if((sockfd=socket(AF_INET,SOCK_STREAM, 0)) == -1)
    {
        printf("socket() error\n");
        exit(1);
    }
    
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);
    
    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        printf("connect() error\n");
        exit(1);
    }
    
    char *s = "\0?{\"mark\":2, \"md5\":\"abcd\", \"size\":198, \"offset\":0, \"threadNum\":1}";
    char* str = "\0?";
    
    if((num = send(sockfd, s, 65, 0)) == -1)
    {
        printf("send() error\n");
        exit(1);
    }
    printf("num:%d\n", num);
    char buffer[200];
    recv(sockfd, buffer, 200, 0);
    printf("buffer:%s\n", buffer);
 //   if((num = send(sockfd, s, 63, 0)) == -1)
 //   {
 //       printf("send() error\n");
 //       exit(1);
 //   }
    sleep(5);

    //sleep(20);

    return 0;
}
