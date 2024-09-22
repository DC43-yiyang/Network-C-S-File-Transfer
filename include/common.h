#ifndef __COMMON_H__
#define __COMMOM_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "md5.h"

#define M                   (1024*1024)
#define BUFFER_SIZE         1024            

#define IP                  "127.0.0.3"
#define PORT                9527            
#define UDP_PORT            8000            
#define LISTENQ             666

#define FILE_MAX            64
#define FILENAME_MAXLEN     64

#define EMPTY               0
#define EXIT                1
#define LS                  2                   
#define GET                 3
#define PUT                 4
#define TCP                 5
#define UDP                 6
#define UNKNOWN             7

#define DEFAULT             "\033[0m"
#define RED                 "\e[0;31m"
#define BLUE                "\e[0;34m"
#define BLACK               "\e[0;30m"

#define MIN(a, b)           (((a) < (b)) ? (a) : (b))  


struct fileinfo{
    char    filename[FILENAME_MAXLEN];      
    int     filesize;                       

    char    md5[33];                        
    int     pos;                            
    int     intact;                         
    int     used;                           
};

struct ip_port{
    char ip[12];
    int  port;
};


struct command{
    char filename[FILENAME_MAXLEN];
    char cmd[4];
    char mode[4];

};

struct PackInfo{
    int   id;
    int   buf_size;
};


struct RecvPack{
    struct PackInfo head;
    char            buf[BUFFER_SIZE];
};

int Socket(int family,int type,int protocol);
void Bind(int fd, const struct sockaddr *sa, socklen_t salen);
void Listen(int fd, int backlog);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
void Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Client_init(struct ip_port *ip_p, int mode);
int Server_init(int mode);


int Open(char *filename);
int Writen(int fd, const void *vptr, const int n);
int Readn(int fd, const void *vptr, const int n);
int createfile(char *filename, int size);
size_t get_filesize(char *path);

char *Md5(char * filename, char *md5);


int get_cmd(char *str);


int send_by_udp(int fd, char *seek, int left, struct sockaddr *addr);
int recv_by_udp(int fd, char *seek, struct sockaddr *addr);

void reset_udp_id();

int readable_timeo(int fd, int sec);
int Readable_timeo(int fd, int sec);

void progress_bar(int rate);

#endif

