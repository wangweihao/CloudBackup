/*======================================================
    > File Name: client.c
    > Author: wwh
    > E-mail:  
    > Other :  
    > Created Time: 2016年01月23日 星期六 19时03分08秒
 =======================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>


#define _USE_GNU
#include <fcntl.h>

#define MAXDATASIZE 100

union len{
    char c[2];
    short length;
}l;

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
    
    char *s = "{\"mark\":2, \"md5\":\"a\", \"size\":10838423, \"offset\":0, \"threadNum\":1}";
    l.length = strlen(s);

    if((num = send(sockfd, l.c, 2, 0)) == -1){
        printf("send 2 error\n");
        exit(1);
    }

    if((num = send(sockfd, s, l.length, 0)) == -1)
    {
        printf("send() error\n");
        exit(1);
    }
    printf("num:%d\n", num);
    
    char *name = "./newfile";
    int pipefd[2];
    int ret;

    int filefd = open("newfile", O_RDWR | O_CREAT, 00777);
    if(filefd < 0)
    {
        perror("open file error\n");
        exit(1);
    }
    lseek(filefd, 0, SEEK_CUR);

    char buffer[2048];
    while(1)
    {
        bzero(buffer, 2048);
        if((ret = recv(sockfd, buffer, 2048, 0)) < 0){
            perror("recv error\n");
            exit(1);
        }else if(ret == 0){
            printf("success!\n");
            break;
        }else{
            write(filefd, buffer, 2048);
        }
  //      if((ret = splice(sockfd, NULL, pipefd[1], NULL, 32768, 5)) == -1){
  //          perror("splice() error\n");
  //          exit(1);
  //      }else if(ret == 0){
  //          break;
  //      }
  //      if((ret = splice(pipefd[0], NULL, filefd, NULL, 32768, 5)) == -1){
  //          perror("splice() error\n");
  //          exit(1);
  //      }
    }
    
    
    sleep(5);


    return 0;
}
