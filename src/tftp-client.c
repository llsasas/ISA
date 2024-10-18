/**
 * @file tftp-client.c
 * @author Samuel Cus (xcussa00@fit.vutbr.cz)
 * @brief  ISA Project
 * @date 2023-10-22
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include "messages.h"
#define RECV_RETRIES 5
char *hostname, *destination_path, *filepath;
int port = 69;
int type;
ssize_t blocksize = 512;

/**
 * @brief Additional function to arguments_check(int num, char **argarr), used for checking the -h
 * @param a -h should be passed here, signalling that the hostname argument should follow
 * @param b Hostname of the server
 */
void handle_hostname(char *a, char *b)
{
    if (!strcmp(a, "-h"))
    {
        hostname = b;
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Additional function to arguments_check(int num, char **argarr), used for checking the -d
 * @param a -d should be passed here, signalling that the destination argument should follow
 * @param b the local or destination filepath
 */
void handle_dfilepath(char *a, char *b)
{
    if (!strcmp(a, "-t"))
    {
        destination_path = b;
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Additional function to arguments_check(int num, char **argarr), used for checking the -p
 * @param a -p should be passed here, signalling that the port number is following
 * @param b port number that the server should be running on
 */
void handle_port(char *a, char *b)
{
    if (!strcmp(a, "-p"))
    {
        port = atoi(b);
        if (port > 65535 || port < 0)
        {
            exit(EXIT_FAILURE);
        }
        return;
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Additional function to arguments_check(int num, char **argarr), used for checking the -f
 * @param a -f should be passed here
 * @param b the local or destination filepath
 */
void handle_filepath(char *a, char *b)
{
    if (!strcmp(a, "-f"))
    {
        filepath = b;
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Checks whether the arguments are passed in the correct way
 * @param num Number of arguments
 * @param argarr Array of arguments passed by the user
 */
void arguments_check(int num, char **argarr)
{
    switch (num)
    {
    case 5:
        handle_hostname(argarr[1], argarr[2]);
        handle_dfilepath(argarr[3], argarr[4]);
        type = UPLOAD;
        return;

    case 7:
        handle_hostname(argarr[1], argarr[2]);
        handle_port(argarr[3], argarr[4]);
        handle_dfilepath(argarr[5], argarr[6]);
        type = UPLOAD;
        return;

    case 9:
        handle_hostname(argarr[1], argarr[2]);
        handle_port(argarr[3], argarr[4]);
        handle_filepath(argarr[5], argarr[6]);
        handle_dfilepath(argarr[7], argarr[8]);
        type = DOWNLOAD;
        return;

    default:
        printf("ERROR: Invalid number of arguments passed\n");
        ;
        exit(EXIT_FAILURE);
    }
}
/**
 * @brief Function that creates UDP socket
 * @return Function returns socket if no error occurs during the creation proccess
 */
int create_socket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock <= 0)
    {
        printf("ERROR: socket()\n");
        exit(EXIT_FAILURE);
    }
    return sock;
}

/**
 * @brief Function used by CLIENT for handling the transfer of the data from the server to the client
 * @param socket Source ID
 * @param address Destination address
 * @param slen Adress lenght
 * @param message Structured data type used for storing informations about the messages
 * @param filename The name of the file that we are going to sent
 * @param fd File that we are going to work with
 * @return Number of bytes that have been sent
 */
void client_receive(FILE *fd, struct sockaddr *address, socklen_t slen, int socket, char *filename)
{
    fd = fopen(filename, "w");
    bool end = false;
    tftp_message *message = malloc(sizeof(tftp_message) + blocksize);
    ssize_t x;
    uint16_t block = 0;
    int tiktok;
    while (true)
    {
        for (tiktok = RECV_RETRIES; tiktok; tiktok--)
        {
            // This should be in loop that is handling timeout
            x = receive_message(socket, message, address, &slen, blocksize);
            if (x >= 0 && x < 4)
            {
                // send error
                free(message);
                fclose(fd);
                close(socket);
                remove(filename);
                exit(EXIT_FAILURE);
            }

            // I guess if we received a correct response on time we jump out of this for that is handling timeout, this break is not for while loop
            if (x >= 4)
            {
                break;
            }

            x = send_ack(socket, block, address, slen);
            if (x < 0)
            {
                free(message);
                fclose(fd);
                close(socket);
                remove(filename);
                exit(EXIT_FAILURE);
            }
        }
        if (!tiktok)
        {
            // Transfer timed out
            free(message);
            fclose(fd);
            close(socket);
            remove(filename);
            exit(EXIT_FAILURE);
        }

        block++;
        // Last packet received
        if (x < blocksize)
        {
            end = true;
        }
        if (!opcodes_check_upload(socket, block, message, slen, address))
        {
            free(message);
            fclose(fd);
            close(socket);
            remove(filename);
            exit(EXIT_FAILURE);
        }
        x = fwrite(message->data.data, 1, x - 4, fd);
        if (x < 0)
        {
            free(message);
            fclose(fd);
            close(socket);
            remove(filename);
            exit(EXIT_FAILURE);
        }
        x = send_ack(socket, block, address, slen);
        if (x < 0)
        {
            free(message);
            fclose(fd);
            close(socket);
            remove(filename);
            exit(EXIT_FAILURE);
        }
        // Last packet end transfer
        if (end)
        {
            free(message);
            fclose(fd);
            return;
        }
    }
}

/**
 * @brief Function used by CLIENT for transferring the file to the server
 * @param socket Source ID
 * @param address Destination address
 * @param slen Adress lenght
 * @param message Structured data type used for storing informations about the messages
 * @param filename The name of the file that we are going to sent
 * @return Number of bytes that have been sent
 */
void client_send(struct sockaddr *address, socklen_t slen, int socket, char *filename)
{
    ssize_t x, datalen;
    uint8_t data[blocksize];
    uint16_t block = 0;
    tftp_message *message = malloc(sizeof(tftp_message)+blocksize);
    int tiktok;
    x = receive_message(socket, message, address, &slen, 512);
    if (x >= 0 && x < 4)
    {
        send_error(socket, address, slen, 0, "Invalid message received\n");
        close(socket);
        exit(EXIT_FAILURE);
    }
    free(message);
    while (true)
    {
        tftp_message *message = malloc(sizeof(tftp_message)+blocksize);
        datalen = fread(data, 1, sizeof(data), stdin);
        block++;
        for (tiktok = RECV_RETRIES; tiktok; tiktok--)
        {
            x = send_data(datalen, slen, address, data, block, socket);
            if (x < 0)
            {
                // ERROR sendto() terminate child process
                close(socket);
                exit(EXIT_FAILURE);
            }
            x = receive_message(socket, message, address, &slen, 512);
            if (x >= 0 && x < 4)
            {
                close(socket);
                exit(EXIT_FAILURE);
            }

            // I guess if we received a correct response on time we jump out of this for that is handling timeout, this break is not for while loop
            // This means that we received correct response
            if (x >= 4)
            {
                break;
            }
            if (errno != EAGAIN)
            {
                close(socket);
                exit(EXIT_FAILURE);
            }
        }
        if (!tiktok)
        {
            close(socket);
            exit(EXIT_FAILURE);
        }
        if (!opcodes_check_download(message, block, block, slen, address))
        {
            close(socket);
            exit(EXIT_FAILURE);
        }
        free(message);

        // Last packet sent
        if (datalen < blocksize)
        {
            return;
        }
    }
}

/**
 * @brief Function that sends RRQ or WRQ message, which message is gonna be send depends on macro -> type
 * @param socket source ID
 * @param slen array of arguments
 * @param adress destination adress
 * @param mode defines whether the transfer mode is "octet" or "netascii"
 */
void send_request(char *mode, struct sockaddr *adress, socklen_t slen, int socket)
{
    int datalen = 0;
    char *modePosition;
    if (type == DOWNLOAD)
    {
        datalen = strlen(filepath) + strlen(mode) + 2;
        tftp_message_request *message = malloc(sizeof(tftp_message_request) + datalen);
        message->request.opcode = htons(RRQ);

        strcpy((char *)message->request.filename_and_mode, filepath);

        modePosition = (char *)message->request.filename_and_mode + strlen(filepath) + 1;
        strcpy(modePosition, mode);
        sendto(socket, message, 2 + datalen, 0, adress, slen);
        free(message);
    }
    else
    {
        datalen = strlen(destination_path) + strlen(mode) + 2;
        tftp_message_request *message = malloc(sizeof(tftp_message_request) + datalen);
        message->request.opcode = htons(WRQ);
        strcpy(message->request.filename_and_mode, destination_path);
        modePosition = message->request.filename_and_mode + strlen(destination_path) + 1;
        strcpy(modePosition, mode);
        sendto(socket, message, 2 + datalen, 0, adress, slen);
        free(message);
    }
}

/**
 * @brief Function handling DOWNLOAD process to server
 * @param socket source ID
 * @param slen array of arguments
 * @param adress destination adress
 * @param mode defines whether it is SERVER or CLIENT
 */
void handle_rrq(int socket, socklen_t slen, struct sockaddr *address, char *mode)
{
    send_request(mode, address, slen, socket);
    FILE *fd;
    client_receive(fd, address, slen, socket, destination_path);
}

/**
 * @brief Function handling UPLOAD process to server
 * @param socket source ID
 * @param slen array of arguments
 * @param adress destination adress
 * @param mode defines whether it is SERVER or CLIENT
 */
void handle_wrq(int socket, socklen_t slen, struct sockaddr *address, char *mode)
{
    send_request(mode, address, slen, socket);
    FILE *fd;
    client_send(address, slen, socket, destination_path);
}

/**
 * @brief Main function
 * @param argc number of arguments
 * @param argv array of arguments
 * @return 0 if everything is okay, 1 if error occurrs during the programme
 */
int main(int argc, char **argv)
{
    arguments_check(argc, argv);
    const char *server_hostname = hostname;
    struct hostent *server;
    struct sockaddr_in server_address;
    int socket;
    char *mode = "octet";
    if ((server = gethostbyname(server_hostname)) == NULL)
    {
        printf("Error occurred: no such host as \n");
        return 1;
    }

    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port);
    socklen_t adress_size = sizeof(server_address);
    struct sockaddr *adress = (struct sockaddr *)&server_address;
    socket = create_socket();
    if (type == DOWNLOAD)
    {
        handle_rrq(socket, adress_size, adress, mode);
    }
    else
    {
        handle_wrq(socket, adress_size, adress, mode);
    }
    // Ends the communication with the server
    if (close(socket) < 0)
    {
       printf("Error occurred: close\n");
        return 1;
    }

    return 0;
}
