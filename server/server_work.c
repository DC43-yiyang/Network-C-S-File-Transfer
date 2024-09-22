#include "../include/server_work.h"

int                 p_id = 0;           //put 
int                 g_id = 0;           //get 

struct fileinfo     p_files[FILE_MAX];  //put file
struct fileinfo     g_files[FILE_MAX];  //client file
struct fileinfo     file;               
extern int listenfd;

struct sockaddr_in  cliaddr;
socklen_t           clilen= sizeof(struct sockaddr_in);

void *broad(){
    struct ip_port  ip;
    strcpy(ip.ip, IP);
    ip.port=PORT;

    int ip_len = sizeof(struct ip_port);
    char msg[ip_len+1];
    bzero(msg,ip_len+1);
    memcpy(msg, &ip, ip_len);

    int brdcFd = Socket(PF_INET, SOCK_DGRAM, 0);

    int optval = 1;
    setsockopt(brdcFd, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));
    struct sockaddr_in theirAddr;
    memset(&theirAddr, 0, sizeof(struct sockaddr_in));
    theirAddr.sin_family = AF_INET;
    theirAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    theirAddr.sin_port = htons(4001);
    for (;;){
        int sendBytes;
        if((sendBytes = sendto(brdcFd, msg, sizeof(msg), 0, (struct sockaddr *)&theirAddr, sizeof(struct sockaddr))) == -1){
            printf("sendto fail, errno=%d\n", errno);
            break;
        }
        //timer += 3;
        sleep(3);
    }
    close(brdcFd);
    return (void*)0;
}



void recv_cmd(struct command *cmd){
    int connfd = Accept(listenfd, (struct sockaddr *) &cliaddr, (socklen_t *)&clilen);   
    int cmdlen = sizeof(struct command);
    char buf[cmdlen+1];
    bzero(buf, cmdlen+1);
    Readn(connfd, buf, cmdlen);
    memcpy(cmd, buf, cmdlen);
    close(connfd);
    return;
}


void recv_fileinfo(struct command *cmd){
    printf("############ recv File information ############\n");
    bzero(&file, sizeof(struct fileinfo));    
    int connfd = Accept(listenfd, (struct sockaddr *) &cliaddr, (socklen_t *)&clilen); 

    char            fileinfo_buf[256] ={'\0'};
    int             fileinfo_len = sizeof(struct fileinfo);
    Readn(connfd, fileinfo_buf, fileinfo_len);

    memcpy(&file, fileinfo_buf, fileinfo_len);
    printf("file from client:\n");
    printf("\t%s\n\tsize=%dM\n\tmd5=%s\n\tlseek=%d\n", file.filename, file.filesize/M, file.md5, file.pos);
    if (get_cmd(cmd->cmd)==PUT){
        for (int i=0; i<FILE_MAX; i++ ){
            if ( strcmp(file.filename, p_files[i].filename)==0 ){
                if (Open(file.filename)==-1)
                    break;
                p_id = i;
                file.pos=p_files[i].pos;
                file.used=1;
                break;
            }        
        }
        if (file.used==0){
            createfile(file.filename, file.filesize);
            file.used=1;
            while (p_files[p_id].used){
                   ++p_id;
                   p_id = p_id % FILE_MAX;
            }
            memcpy(&p_files[p_id], &file, fileinfo_len);
        }
        printf("Saving: p_id:%d %s %d %s lseek=%d\n",p_id, p_files[p_id].filename, p_files[p_id].filesize, p_files[p_id].md5, p_files[p_id].pos);
    }
    else{
        for (int i=0; i<FILE_MAX; i++ ){
            if (strcmp(file.filename, g_files[i].filename)==0){
                g_id = i;
                memcpy(&file, &g_files[i], fileinfo_len);
                file.used=1;
                break;
            }
        }
        
        if (file.used==0){
            printf("404 Not Found\n");
        }

    }

    char            send_buf[256] ={'\0'};
    memcpy(send_buf , &file, fileinfo_len);
    printf("file to client:\n");
    printf("\t%s\n\tsize=%dM\n\tmd5=%s\n\tlseek=%d\n", file.filename, file.filesize/M, file.md5, file.pos);
    Writen(connfd, send_buf,  fileinfo_len);
    printf("########### send back File information ###########\n");
    close(connfd);
    return;
}


void recv_block(struct command *cmd){
    printf("\n############ Receive File ############\n");
    int b_tcp = get_cmd(cmd->mode)==UDP? UDP:TCP;
    
    int fd = Open(file.filename);
    char *begin=(char *)mmap(NULL, file.filesize, PROT_WRITE|PROT_READ, MAP_SHARED, fd , 0);
    close(fd);
    char *seek = begin + file.pos;


    int n=0;
    int left = file.filesize -  file.pos;
    if (left < 1){
        printf("Send it\n");
        goto over;
    }
    if (b_tcp==TCP){
        printf("======== TCP ========\n");
        int connfd = Accept(listenfd, (struct sockaddr *) &cliaddr, (socklen_t *)&clilen); 
        while(left>0){
            n=read(connfd, seek, left);
            if (n<0) {
                printf("Can't connect\n");
                break;
            }
            file.pos += n;
            p_files[p_id].pos += n;
            left -= n;
            seek +=n ;
            printf("recv:%d  left:%dM  lseek:%d\n",n, left/M, p_files[p_id].pos);
            progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }
        printf("======= TCP OK =======\n");
        close(connfd);
    }else{
        printf("======== UDP ========\n");
        bzero(&cliaddr, clilen);
        int connfd = Server_init(UDP);
        //sleep(1);
        while(left > 0){             
            n=recv_by_udp(connfd, seek, (struct sockaddr *)&cliaddr);
            if (n==-1)break;
            file.pos += n;
            p_files[p_id].pos += n;
            left -= n;
            seek += n;
            //printf("#3 recv:%d  left:%dM  lseek:%d\n",n, left/M, file.pos);
            progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }
        printf("======= UDP OK =======\n");
        reset_udp_id();
        close(connfd);
    }
over:
    munmap((void *)begin, file.filesize);
    
    printf("Finish upload chech the file ..\n");
    char    md5[33] = {'\0'};
    Md5(file.filename, md5);

    if (strcmp(file.md5,md5)==0){
        printf("This file is complete！\n");
        printf("Finished upload 100%%\n");
        p_files[p_id].intact=1;
        p_files[p_id].pos=file.filesize;

        
        int b_already=0;
        for (int i=0; i<FILE_MAX; i++ ){
            if ( strcmp(file.filename, g_files[i].filename)==0 ){
                g_id=i;
                b_already=1;
                break;
            }        
        }
        if (b_already ==0 ){
            while(g_files[g_id].used==1){
                 ++g_id;
                g_id = g_id % FILE_MAX;
            }
            memcpy(&g_files[g_id], &p_files[p_id], sizeof(struct fileinfo));
            g_files[g_id].pos=0;
        }

        printf("Finish Upload, the file located: g_files[%d]\n%s %d MD5:%s lseek=%d\n",g_id, g_files[g_id].filename, g_files[g_id].filesize, g_files[g_id].md5, g_files[g_id].pos);
    }else{
        printf("This file crashed...\n");
        printf("Upload : %d%%\n", (int)((100*(p_files[p_id].pos/M))/(p_files[p_id].filesize/M)) );
    }
    
    printf("############ Receive Finish ############\n");
    return;
}


void send_file(struct command *cmd){
    if (file.used==0){
        printf("can't find %s !\n",file.filename);
        return;
    }

    int fd = Open(cmd->filename);
    if (fd==-1){
        printf("can't find %s !\n",file.filename);
        return;
    }
    printf("\n############ Send File ############\n");      
    printf("%s size:%d MD5:%s Download Location:%d\n",file.filename, file.filesize, file.md5, file.pos);
    char *begin=(char *)mmap(NULL, file.filesize, PROT_WRITE|PROT_READ, MAP_SHARED, fd , 0);
    close(fd);
    int n=0;
    int left = file.filesize -  file.pos;
    if (left < 1){
        printf("Finish \n");
        goto over;
    }
    char *seek = begin + file.pos;
    int b_tcp = get_cmd(cmd->mode)==UDP? UDP:TCP;
    if (b_tcp==TCP){
        printf("======== TCP ========\n");

        int connfd = Accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
        while(left>0){
            int bytes = MIN(left, M);
            n=write(connfd, seek, bytes);
            left -= n;
            g_files[g_id].pos += n;
            file.pos += n;
            seek += n;
            printf("send:%d  left:%dM  lseek:%d\n",n, left/M, g_files[g_id].pos);
            progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }
        printf("====== TCP Send Finish ======\n");
        close(connfd);
    }
    else{
        printf("======== UDP ========\n");
        bzero(&cliaddr, clilen);
        reset_udp_id();
        int connfd = Server_init(UDP);
        
        char buf[10]={'\0'};
        socklen_t               clilen= sizeof(struct sockaddr_in);
        recvfrom(connfd, buf, 10, 0, (struct sockaddr*)&cliaddr,&clilen);
        printf("%s...\n",buf);
        
        while(left > 0){
            n=send_by_udp(connfd, seek, left, (struct sockaddr*)&cliaddr);
            if (n == -1)break;
            file.pos += n;
            g_files[g_id].pos += n;
            left -= n;
            seek += n;
            //printf("#3 Send:%d  left:%dM  lseek:%d\n",n, left/M, g_files[g_id].pos);
            progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }
        printf("====== UDP Send Finish ======\n");
        close(connfd);
    }
over:    
    printf("Download %d%%\n", (int)((100*(g_files[g_id].pos/M))/(g_files[g_id].filesize/M)) );
    //if (left <= 0)
        //g_files[g_id].pos=0;
    munmap((void *)begin, file.filesize);
    printf("############ Send Finish ############\n");
    
    return;
}


