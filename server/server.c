#include "server_work.h"

int listenfd;

int main(){
    /** Initialize the broadcast thread **/
    pthread_t broadthrd;
    int err=-1;
    if (( (err = pthread_create(&broadthrd,NULL,broad,NULL))) !=0 ){
        perror("can't create broad thread");
        exit(-1);
    }

    listenfd = Server_init(TCP);
    
    struct command cmd;
    while(1){
        printf("\nWait for task...\n");
        bzero(&cmd,sizeof(struct command));
        recv_cmd(&cmd);
        printf("cmd:%s -%s -%s\n",cmd.filename,cmd.cmd,cmd.mode); 
        recv_fileinfo(&cmd); 

        switch (get_cmd(cmd.cmd)){

            case GET:
                send_file(&cmd);
                break;
            case PUT:
ß                recv_block(&cmd);
                break;
            case UNKNOWN:
            default:
                printf("cmd error\n");
                break;
        }
    }

    return 0;
}


