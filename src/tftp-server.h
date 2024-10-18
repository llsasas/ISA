/**
 * @file tftp-server.h
 * @author Samuel Cus (xcussa00@fit.vutbr.cz)
 * @brief  ISA Project
 * @date 2023-10-22
 */
#ifndef TFTP_SERVER_H
#define TFTP_SERVER_H
#include "messages.h"

enum MODE
{
    OCTET,
    NETASCII
};

void check_args(int argscount, char **args);

void server(int sck);

void handle_client_rqst(tftp_message_request *msg, struct sockaddr *adress, socklen_t len, ssize_t lenght, int socket);

void check_args(int argscount, char **args);

void server_bind(int server_socket);

int create_socket();
#endif
