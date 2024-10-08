#include "client_work.h"

struct fileinfo file;

struct sockaddr_in  serv_addr;

void menu(struct ip_port *ip){
    printf("************************************************************\n");
    printf("*           Welcome to server: %s:%d              *\n",ip->ip,ip->port);
    printf("*                                                          *\n");
    printf("*  1.download files in TCP mode: file -get -t              *\n");
    printf("*  2.download files in UDP mode: file -get -u              *\n");
    printf("*  3.upload files in TCP mode: file -put -t                *\n");
    printf("*  4.upload files in UDP mode: file -put -u                *\n");
    printf("*  5.exit: q quit or exit                                  *\n");
    printf("************************************************************\n");
}


void get_server_list(struct ip_port *ip_p){
    printf("searching available servers...\n");
    int sockListen = Socket(AF_INET, SOCK_DGRAM, 0);

    int set = 1;
    setsockopt(sockListen, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(int));
    struct sockaddr_in recvAddr;
    memset(&recvAddr, 0, sizeof(struct sockaddr_in));
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(4001);
    recvAddr.sin_addr.s_addr = INADDR_ANY;
    Bind(sockListen, (struct sockaddr *)&recvAddr, sizeof(struct sockaddr));

    int recvbytes;
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int ip_len = sizeof(struct ip_port);
    struct ip_port servers[16];
    int indx=0;
    struct ip_port ip;            
    char msg[ip_len+1];
    struct  timeval start;
    struct  timeval end;
    gettimeofday(&start,NULL);
    for(;;){
recv:
        bzero(&ip,ip_len);
        bzero(msg,ip_len+1);

        if((recvbytes = recvfrom(sockListen, msg, 128, 0,(struct sockaddr *)&recvAddr, &addrLen)) != -1) {
            gettimeofday(&end,NULL);
            int timer=end.tv_sec-start.tv_sec;
            if (timer > 5 || indx > 15)
                break;
            memcpy(&ip, msg, recvbytes);
            for (int i=0; i<indx; i++){
                if(strcmp(ip.ip, servers[i].ip)==0){
                    goto recv;
                }                
            }

            strcpy(servers[indx].ip, ip.ip);
            servers[indx].port = ip.port;            

            indx += 1;
        }else{
            printf("recvfrom timeout\n");
            break;
            //exit(0);
        }
    }
  
    close(sockListen);
    
    printf("============== available servers ==============\n");    
    for (int i=1;i<indx;i++)
        printf("\t[%d]IP:%s  port:%d\n", i, servers[i].ip, servers[i].port);
    printf("===============================================\n");
    printf("which one do you want to connect? ");
    int slct;
input:    
    scanf("%d",&slct);
    if (slct>=0 && slct<indx){
        strcpy(ip_p->ip, servers[slct].ip);
        ip_p->port=servers[slct].port;
        printf("you select: %s : %d\n",ip_p->ip, ip_p->port);
    }else{
        printf("invalid input!retry...\n");
        goto input;
    }
    getchar();
    return;
}



int split(struct command *cmd, char *line){

    if (cmd==NULL||line==NULL)return -1;


    if (strlen(line) > 256){
         printf("command is too long!\n");
         return -1;
    }

    char *pch;
    pch = strtok(line," -,");
    int index=0;

    while(pch!=NULL){
        if (index==0)
            strcpy(cmd->filename,pch);
        if (index==1)
            strcpy(cmd->cmd,pch);
        if (index==2)
            strcpy(cmd->mode,pch);
        pch = strtok(NULL, " -,");
        index++;
        if (index>2)
            break;
    }
    cmd->cmd[3]='\0';
    cmd->mode[3]='\0';

    int b_exit = strcmp(cmd->filename,"q")==0 || strcmp(cmd->filename,"quit")==0 || strcmp(cmd->filename,"exit")==0  ;
    if ( index == 1 && !b_exit ){
        printf("input error!\n");
        return -1;
    }

    if (index >1 && strcmp(cmd->cmd,"put")==0 && Open(cmd->filename) ==-1){
        printf("Can't find %s!\n",cmd->filename);
        return -1;
    }
    int b_get = strcmp(cmd->cmd,"get")==0 || strcmp(cmd->cmd,"put")==0 ;
    if ( index==2 && !b_get ){
        printf("put or get error!\n");
        return -1;
    }

    int b_mod = strcmp(cmd->mode,"t")==0 || strcmp(cmd->mode,"tcp")==0 || strcmp(cmd->mode,"u")==0  || strcmp(cmd->mode,"udp")==0 ;
    if ( index==3 ){
        if (!b_get){
            printf("put or get error!\n");
            return -1;
        }
        if (!b_mod){
            printf("tcp or udp error!\n");
            return -1;
        }
    }
    return 0;
}

char *input(char *str){
    str=readline(RED"cmd"DEFAULT"@"BLUE"ubuntu:"BLACK"~$ "DEFAULT);
    if(str && *str)
        add_history(str);
    return str;
}

void get_input(struct command *cmd, struct ip_port *ip){
    bzero(&file, sizeof(struct fileinfo));
    char inputline[1024] = {'\0'};
    char *cmdline = NULL;
getcmd:
    bzero(cmd,sizeof(struct command));
    cmdline = input(inputline);
    int err = split(cmd, cmdline);
    if (err != 0)
        goto getcmd;

    exec_cmd(cmd,ip);
}


void exec_cmd(struct command *cmd, struct ip_port *ip){
    if (get_cmd(cmd->filename)==EXIT)
        exit(0);
    send_cmd(cmd, ip);    
    send_fileinfo(cmd,ip);
    
    switch (get_cmd(cmd->cmd)){
        case GET:
            recv_file(cmd,ip);break; 

        case PUT:            
            send_block(cmd,ip);break;
   
        case UNKNOWN:get_input(cmd, ip);break;
        default:get_input(cmd, ip);break;
    }
}

void send_cmd(struct command *cmd, struct ip_port *ip){
    usleep(100000);
    int sock_fd = Client_init(ip, TCP);
    int cmdlen = sizeof(struct command);
    char buf[cmdlen+1];
    bzero(buf, cmdlen+1);
    memcpy(buf, cmd, cmdlen);   
    Writen(sock_fd,buf,cmdlen);
    close(sock_fd);
}

void send_fileinfo(struct command *cmd, struct ip_port *ip){
    printf("\n########### Send file information ###########\n");
    usleep(100000);
    int sock_fd = Client_init(ip, TCP);
    strcpy(file.filename, cmd->filename);

    if (get_cmd(cmd->cmd)==PUT){
        file.filesize = get_filesize(cmd->filename);
        char    md5[33] = {'\0'};
        Md5(file.filename, md5);
        strcpy(file.md5, md5);
    }
    file.pos=0;
    file.intact=0;
    file.used=0;

    char    send_buf[256] ={'\0'};

    int     fileinfo_len = sizeof(struct fileinfo);
    memcpy(send_buf , &file, fileinfo_len);
    Writen(sock_fd, send_buf,  fileinfo_len);

    printf("=== To-->\n\t%s\n\tsize=%dM\n\tmd5=%s\n\tlseek=%d\n", file.filename, file.filesize/M, file.md5, file.pos);

    bzero(send_buf, 256);
    bzero(&file,fileinfo_len);
    Readn(sock_fd, send_buf, fileinfo_len);
    memcpy(&file, send_buf, fileinfo_len);
    printf("== Recv-->\n\t%s\n\tsize=%dM\n\tmd5=%s\n\tlseek=%d\n", file.filename, file.filesize/M, file.md5, file.pos);
    printf("############ Send Finish ############\n");
    close(sock_fd);
    return;
}


void recv_file(struct command *cmd, struct ip_port *ip){
    int b_tcp = get_cmd(cmd->mode)==UDP? UDP:TCP;
    if (file.used==0){
        printf("server: 404 Not Found\n");
        get_input(cmd, ip);
    }
    printf("\n############ Start download ############\n");   
    printf("%s size:%d MD5:%s download loc:%d\n",file.filename, file.filesize, file.md5, file.pos);

    char path[100]={'\0'};
    getcwd(path, sizeof(path));
    strcat(path, "/download/");
    
    if (access(path, 0)==-1){
        printf("mkdir %s\n", path);
        if (mkdir(path, 0777))
            printf("mkdir %s failed!!\n", path);
    }
    strcat(path, file.filename);
    printf("location: %s\n", path);
    if (file.pos==0)
        createfile(path, file.filesize);

    int fd = Open(path);
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
        usleep(100000);
        int sock_fd = Client_init(ip, TCP);
        while(left>0){
            n=read(sock_fd, seek, left);
            file.pos += n;
            left -= n;
            seek += n;
            printf("recv:%d  left:%dM  lseek:%d\n",n, left/M, file.pos);
            if(file.filesize > M)
                progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }
        printf("======= TCP OK =======\n");
        close(sock_fd);
    }
     else{
         printf("======== UDP ========\n");
         udp_serv_init(&serv_addr, ip);
         int sock_fd = Socket(AF_INET, SOCK_DGRAM, 0);
         
         char buf[10]="Download";
         int  serv_len= sizeof(struct sockaddr_in);
         sendto(sock_fd, buf, 10,0,(struct sockaddr*)&serv_addr,serv_len);
        if (Readable_timeo(sock_fd, 8)==0){
                printf("Timeout!\n");
                reset_udp_id();
                close(sock_fd);
                munmap((void *)begin, file.filesize);
                get_input(cmd, ip);
        }
         //usleep(100000);
         while(left > 0){
             n = recv_by_udp(sock_fd, seek, (struct sockaddr *)&serv_addr);
             if (n==-1)break;
             file.pos += n;
             left -= n;
             seek += n;
             //printf("#3 recv:%d  left:%dM  lseek:%d\n",n, left/M, file.pos);
             if(file.filesize > M)
                progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
         }
         printf("======= UDP OK =======\n");
         reset_udp_id();
         close(sock_fd);
     }
over:
     munmap((void *)begin, file.filesize);
     printf("Download finish, check file whether complete...\n");
     char    md5[33] = {'\0'};
     Md5(path, md5);

     if (strcmp(file.md5, md5)==0){
         printf("This file is complete\n");
         printf("Finish download 100%%\n");
     }else{
         printf("This file crashed...\n");
         if(file.filesize > M)
            printf("Download location %d%%\n", (int)((100*(file.pos/M))/(file.filesize/M)) );
     }   
     
     printf("############ Download finish ############\n"); 
     get_input(cmd, ip);
}

void send_block(struct command *cmd, struct ip_port *ip){
    printf("\n############ Start upload ############\n");
    int b_tcp = get_cmd(cmd->mode)==UDP? UDP:TCP;
    
    int fd = Open(cmd->filename);
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
       printf("======== TCP ========!\n");
       usleep(100000);
       int sock_fd = Client_init(ip, TCP);
       while(left>0){
            int bytes = MIN(left, M);
            n=write(sock_fd, seek, bytes);
            left -= n;
            seek += n;
            printf("send:%d  left:%dM\n",n, left/M);
            if(file.filesize > M)
                progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }   
        printf("======= TCP OK =======!\n");
        close(sock_fd);
    }
    else{
        printf("======== UDP ========\n");
        reset_udp_id();
        udp_serv_init(&serv_addr, ip);
        int sock_fd = Socket(AF_INET, SOCK_DGRAM, 0);

        while(left>0){
            n = send_by_udp(sock_fd, seek, left, (struct sockaddr *)&serv_addr);
            left -= n;
            seek += n;
            //printf("#3 Send:%d  left:%dM\n", n, left/M);
            if(file.filesize > M)
                progress_bar((100*((file.filesize-left)/M))/(file.filesize/M));
        }   
        printf("======= UDP OK =======\n");
        close(sock_fd);
    }
over:
    printf("############ Upload finish ############\n");
    munmap((void *)begin, file.filesize);
    get_input(cmd, ip);

}

void udp_serv_init(struct sockaddr_in *server_addr, struct ip_port *ip){
    bzero(server_addr, sizeof(struct sockaddr_in));
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr(ip->ip);
    server_addr->sin_port = htons(UDP_PORT);
}


