#ifndef __CLIENT_WORK_H__
#define __CLIENT_WORK_H__
#include "common.h"

/*************************************************************************
* @function name:           menu
* @function description:    Display menu, command help prompts
* @input:                   Address of the ip_port structure
* @output:                  None
* @return:                  None
 ************************************************************************/
void menu(struct ip_port *ip);


/*************************************************************************
* @function name:           get_server_list
* @function description:    Get the list of available servers in the local network
* @input:                   Address of the ip_port structure
* @output:                  Print the list of available servers, including server number, IP, and port
* @return:                  None
 ************************************************************************/
void get_server_list(struct ip_port *ip);


/*************************************************************************
* @function name:           split
* @function description:    Parse the command line arguments to get the filename, upload/download, TCP/UDP mode, exit information, etc.
* @input:                   Command structure, a line from the terminal's standard input
* @output:                  Store the parsed parameters into the struct variable cmd
* @return:                  Returns 0 on success, -1 on failure
 ************************************************************************/
int split(struct command *cmd, char *line);

/** Get input command, can use tab for autocomplete, up/down keys for command history **/
char *input(char *str);

/*************************************************************************
* @function name:           get_input
* @function description:    Get input parameters from the terminal and call function split to parse
* @input:                   Address of the ip_port structure, cmd structure
* @output:                  None
* @return:                  None
 ************************************************************************/
void get_input(struct command *cmd, struct ip_port *ip);


/*************************************************************************
* @function name:           exec_cmd
* @function description:    Execute related operations based on the ip and the obtained command line
 ************************************************************************/
void exec_cmd(struct command *cmd, struct ip_port *ip);


/** Send command to the server **/
void send_cmd(struct command *cmd, struct ip_port *ip);


/** Send file information to the server, return information for resuming transfer **/
void send_fileinfo(struct command *cmd, struct ip_port *ip);


/** Download file **/
void recv_file(struct command *cmd, struct ip_port *ip);


/** Upload file **/
void send_block(struct command *cmd, struct ip_port *ip);


/** Initialize UDP server **/
void udp_serv_init(struct sockaddr_in *server_addr, struct ip_port *ip);

#endif
