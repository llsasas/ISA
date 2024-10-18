/**
 * @file tftp-client.h
 * @author Samuel Cus (xcussa00@fit.vutbr.cz)
 * @brief  ISA Project
 * @date 2023-10-22
 */
#ifndef TFTP-CLIENT_H
#define TFTP_CLIENT_H

void handle_hostname(char *a, char *b);

void handle_dfilepath(char *a, char *b);

void handle_port(char *a, char *b);

void handle_filepath(char *a, char *b);

void arguments_check(int num, char **argarr);

void send_request( char *mode, struct sockaddr *adress, socklen_t slen, int socket);

void handle_rrq(int socket, socklen_t slen, struct sockaddr *adress);

void handle_wrq(int socket, socklen_t slen, struct sockaddr *adress);


#endif