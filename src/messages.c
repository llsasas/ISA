/**
 * @file messages.c
 * @author Samuel Cus (xcussa00@fit.vutbr.cz)
 * @brief  ISA Project
 * @date 2023-10-22
 */
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include "messages.h"
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>

/**
 * @brief Function used by both SERVER and CLIENT for printing output on stdeer as describet in requierements
 * @param socket Destination ID
 * @param address Destination address
 * @param message data type used for storing the information about the message
 */
void message_info(tftp_message *message, struct sockaddr *address, int socket)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(socket, (struct sockaddr *)&addr, &addrlen) == -1)
    {
        printf("ERROR getsockname()\n");
        exit(EXIT_FAILURE);
    }
    int destination_port = ntohs(addr.sin_port);
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in *addr_in = (struct sockaddr_in *)address;
    (void)inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN);
    int source_port = ntohs(addr_in->sin_port);
    char *filename, mode;
    switch ((ntohs(message->opcode)))
    {
    case ACK:
        fprintf(stderr, "ACK %s:%d %d\n", ip_str, source_port, ntohs(message->ack.block_number));
        break;
    case DATA:
        fprintf(stderr, "DATA %s:%d:%d %d\n", ip_str, source_port,destination_port, ntohs(message->data.block_number));
        break;
    case ERROR:
        fprintf(stderr, "ERROR %s:%d:%d %d \"%s\"\n", ip_str, source_port, destination_port, ntohs(message->error.error_code), message->error.error_string);
        break;
    }
}


/**
 * @brief Function used by both SERVER and CLIENT for receiving message
 * @param socket Source ID
 * @param address Destination address
 * @param slen Address lenght
 * @param message data type used for storing the information about the message
 * @return Number of bytes received
 */
ssize_t receive_message(int socket, tftp_message *message, struct sockaddr *address, socklen_t *slen, ssize_t size)
{
    ssize_t bsize = recvfrom(socket, message, sizeof(message)+size, 0, address, slen);
    if (bsize < 0)
    {
        printf("ERROR recvfrom()\n");
        return bsize;
    }
    else
    {
        message_info(message, address, socket);
        return bsize;
    }
}

/**
 * @brief Function used by both SERVER and CLIENT for sending ACK message
 * @param socket Source ID
 * @param address Destination address
 * @param slen Address lenght
 * @param block Defines for what block of the data we send the ACK for
 * @return Number of bytes that have been sent
 */
ssize_t send_ack(int socket, uint16_t block, struct sockaddr *address, socklen_t len)
{
    tftp_message message;
    ssize_t x;
    message.ack.opcode = htons(ACK);
    message.ack.block_number = htons(block);

    if ((x = sendto(socket, &message, sizeof(message.ack), 0, address, len)) < 0)
    {
        printf("ERROR sendto()\n");
    }
    return x;
}


/**
 * @brief Function used by both SERVER and CLIENT for sending the ERROR message
 * @param socket Source ID
 * @param address Destination address
 * @param slen Address lenght
 * @param error_msg String containing message describing the error
 * @param error Number that specifies which error occurred
 * @return Number of bytes that have been sent
 */
ssize_t send_error(int socket, struct sockaddr *address, socklen_t len, int error, char *error_msg)
{
    tftp_message *message = malloc(sizeof(tftp_message) + strlen(error_msg));
    ssize_t x;
    if (strlen(error_msg) > 512)
    {
        printf("ERROR: error msg too long\n");
        return -1;
    }
    message->error.opcode = htons(ERROR);
    message->error.error_code = htons(error);
    strcpy(message->error.error_string, error_msg);

    if (x = sendto(socket, message, strlen(error_msg) + 4, 0, address, len) < 0)
    {
        printf("ERROR sendto()\n");
    }
    free(message);
    return x;
}

/**
 * @brief Function used by both SERVER and CLIENT for sending OACK message
 * @param socket Source ID
 * @param address Destination address
 * @param slen Address lenght
 * @param block Defines for what block of the data we send the OACK for
 * @param options Options that are about to be OACK
 * @return Number of bytes that have been sent
 */
ssize_t send_oack(int socket, uint16_t block, struct sockaddr *address, socklen_t len, char *options)
{
    ssize_t x;
    tftp_message *message = malloc(sizeof(tftp_message) + 512);
    message->oack.opcode = htons(OACK);
    message->oack.block_number = htons(block);
    strcpy(message->oack.options, options);
    int num = 1;
    int lenght = strlen(options);
    options = strchr(options, '\0') + 1;
    while (options[0] != '\0')
    {
        strcpy(message->oack.options+lenght+num, options);
        num++;
        lenght += strlen(options);
        options = strchr(options, '\0') + 1;
    }
    if ((x = sendto(socket, message, lenght + 4 + num, 0, address, len)) < 0)
    {
        printf("ERROR sendto()\n");
    }
    free(message);
    return x;
}

/**
 * @brief Function used by both SERVER and CLIENT for sending the data
 * @param socket Source ID
 * @param address Destination address
 * @param slen Adress lenght
 * @param block Defines the number of the block that is about to be sent
 * @param len The size of the DATA that is to be sent
 * @return Number of bytes that have been sent
 */
ssize_t send_data(ssize_t len, socklen_t slen, struct sockaddr *address, uint8_t *data, uint16_t block, int socket)
{
    tftp_message *message = malloc(sizeof(tftp_message) + len);
    message->data.opcode = htons(DATA);
    message->data.block_number = htons(block);
    memcpy(message->data.data, data, len);
    ssize_t x;
    if ((x = sendto(socket, message, len + 4, 0, address, slen)) < 0)
    {
        printf("ERROR sendto()\n");
    }
    free(message);
    return x;
}

/**
 * @brief Function used by both SERVER and CLIENT for checking the opcodes received
 * @param socket Source ID
 * @param address Destination address
 * @param slen Adress lenght
 * @param message Structured data type used for storing informations about the messages
 * @param block Used to check if the block numbers correspond
 * @return True if opcodes and block numbers match
 */
bool opcodes_check_download(tftp_message *message, int socket, uint16_t block, socklen_t slen, struct sockaddr *address)
{
    if (ntohs(message->opcode) == ERROR)
    {
        printf("Error message received: %s", message->error.error_string);
        return false;
    }
    if (ntohs(message->opcode) != ACK)
    {
        // Received invalid message during transfer process
        send_error(socket, address, slen, 0, "Invalid message received during transfer\n");
        return false;
    }
    if (ntohs(message->ack.block_number) != block)
    {
        // Block number does not match
        send_error(socket, address, slen, 0, "Invalid ACK number received\n");
        return false;
    }
    return true;
}

/**
 * @brief Function used by both SERVER and CLIENT for checking the opcodes received
 * @param socket Source ID
 * @param address Destination address
 * @param slen Adress lenght
 * @param message Structured data type used for storing informations about the messages
 * @param block Used to check if the block numbers correspond
 * @return True if opcodes and block numbers match
 */
bool opcodes_check_upload(int socket, uint16_t block, tftp_message *message, socklen_t slen, struct sockaddr *address)
{
    if (ntohs(message->opcode) == ERROR)
    {
        printf("Error message received: %s", message->error.error_string);
        return false;
    }
    if (ntohs(message->opcode) != DATA)
    {
        // Received error message terminate child process
        send_error(socket, address, slen, 0, "Invalid message received during transfer\n");
        close(socket);
        return false;
    }
    if (ntohs(message->ack.block_number) != block)
    {
        // Block number does not match
        send_error(socket, address, slen, 0, "Invalid ACK number received\n");
        close(socket);
        return false;
    }
    return true;
}