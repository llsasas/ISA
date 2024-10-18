/**
 * @file messages.h
 * @author Samuel Cus (xcussa00@fit.vutbr.cz)
 * @brief  ISA Project
 * @date 2023-10-22
 */
#ifndef MESSAGES_H
#define MESSAGES_H
#include <stdbool.h>

enum OPCODES
{
    RRQ = 1,
    WRQ,
    DATA,
    ACK,
    ERROR,
    OACK
};

enum TYPE
{
    UPLOAD,
    DOWNLOAD
};

enum ERRORS
{
    not_defined,
    file_not_found,
    acces_violation,
    disk_full,
    illegal_operation,
    unknown_id,
    file_exists,
    unknown_user,
    option_negogiaton
};

typedef union
{
    uint16_t opcode;

    struct
    {
        uint16_t opcode; /* DATA */
        uint16_t block_number;
        uint8_t data[];
    } data;

    struct
    {
        uint16_t opcode; /* ACK */
        uint16_t block_number;
    } ack;

    struct
    {
        uint16_t opcode; /* ERROR */
        uint16_t error_code;
        uint8_t error_string[];
    } error;

    struct
    {
        uint16_t opcode; /* OACK */
        uint16_t block_number;
        uint8_t options[];
    } oack;
} tftp_message;

typedef union
{
    uint16_t opcode;
    struct
    {
        uint16_t opcode; /* RRQ or WRQ */
        uint8_t filename_and_mode[];
    } request;

} tftp_message_request;

int create_socket();

ssize_t receive_message_request(int socket, tftp_message_request *message, struct sockaddr *address, socklen_t *slen);

ssize_t receive_message(int socket, tftp_message *message, struct sockaddr *address, socklen_t *slen, ssize_t size);

ssize_t send_ack(int socket, uint16_t block, struct sockaddr *address, socklen_t len);

ssize_t send_error(int socket, struct sockaddr *address, socklen_t len, int error, char *error_msg);

ssize_t send_oack(int socket, uint16_t block, struct sockaddr *address, socklen_t len, char *options);

ssize_t send_data(ssize_t len, socklen_t slen, struct sockaddr *address, uint8_t *data, uint16_t block, int socket);

bool opcodes_check_download(tftp_message *message, int socket, uint16_t block, socklen_t slen, struct sockaddr *address);

bool opcodes_check_upload(int socket, uint16_t block, tftp_message *message, socklen_t slen, struct sockaddr *address);


#endif 