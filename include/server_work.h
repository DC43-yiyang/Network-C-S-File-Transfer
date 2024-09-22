#ifndef __SERVER_WORK_H__
#define __SERVER_WORK_H__
#include "common.h"


void *broad();


void recv_cmd(struct command *cmd);


void recv_fileinfo(struct command *cmd);


void recv_block(struct command *cmd);


void send_file(struct command *cmd);

#endif

