/**
 * @file tftp-server.c
 * @author Samuel Cus (xcussa00@fit.vutbr.cz)
 * @brief  ISA Project
 * @date 2023-10-22
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include "tftp-server.h"
#include "messages.h"
#define PORT 69
#define RECV_RETRIES 5
int port = -1;
char *directory;
int RECV_TIMEOUT = 5;
int tsize = 0;
int blocksize = 512;
int timeout;
struct timeval tv;

/**
 * @brief Function that creates UDP socket
 * @return Function returns socket if no error occurs during the creation proccess
 */
int create_socket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock <= 0)
    {
        printf("ERROR: Socket create\n");
        exit(EXIT_FAILURE);
    }
    return sock;
}

/**
 * @brief Function used by SERVER for printing out message info
 * @param socket Destination ID
 * @param address Destination address
 * @param message data type used for storing the information about the request message
 * @param opts Options to be printout, if it is NULL then no options were attached
 */
void request_message_info(tftp_message_request *message, struct sockaddr *address, int socket, ssize_t bsize)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(socket, (struct sockaddr *)&addr, &addrlen) == -1)
    {
        printf("ERROR: getsockname()\n");
        exit(EXIT_FAILURE);
    }
    int destination_port = ntohs(addr.sin_port);
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in *addr_in = (struct sockaddr_in *)address;
    (void)inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN);
    int source_port = ntohs(addr_in->sin_port);
    char *options = NULL;
    ssize_t lenght = 0;
    switch ((ntohs(message->request.opcode)))
    {
    case WRQ:
        options = message->request.filename_and_mode;
        lenght += strlen(options);
        fprintf(stderr, "WRQ %s:%d \"%s\" ", ip_str, source_port, options);
        options = strchr(options, '\0') + 1;
        lenght += strlen(options);
        fprintf(stderr, "%s", options);
        if (lenght == bsize - 4)
        {
            fprintf(stderr, "\n");
        }
        else
        {
            options = strchr(options, '\0') + 1;
            fprintf(stderr, " ");
            while (options[0] != '\0')
            {
                fprintf(stderr, "%s", options);
                options = strchr(options, '\0') + 1;
                fprintf(stderr, "=%s ", options);
                options = strchr(options, '\0') + 1;
            }
            fprintf(stderr, "\n");
        }
        break;
    case RRQ:
        options = message->request.filename_and_mode;
        lenght += strlen(options);
        fprintf(stderr, "RRQ %s:%d \"%s\" ", ip_str, source_port, options);
        options = strchr(options, '\0') + 1;
        lenght += strlen(options);
        fprintf(stderr, "%s", options);
        if (lenght == bsize - 4)
        {
            fprintf(stderr, "\n");
        }
        else
        {
            options = strchr(options, '\0') + 1;
            fprintf(stderr, " ");
            while (options[0] != '\0')
            {
                fprintf(stderr, "%s", options);
                options = strchr(options, '\0') + 1;
                fprintf(stderr, "=%s ", options);
                options = strchr(options, '\0') + 1;
            }
            fprintf(stderr, "\n");
        }
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
ssize_t receive_message_request(int socket, tftp_message_request *message, struct sockaddr *address, socklen_t *slen)
{
    ssize_t bsize = recvfrom(socket, message, sizeof(message) + 512, 0, address, slen);
    if (bsize < 0)
    {
        printf("ERROR recvfrom()\n");
        return bsize;
    }
    else
    {
        request_message_info(message, address, socket, bsize);
        return bsize;
    }
}

/**
 * @brief Function that binds the server on correct socket and port
 * @param socket The server is going to bind with this socket
 */
void server_bind(int server_socket)
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    struct sockaddr *address = (struct sockaddr *)&server_addr;
    int address_size = sizeof(server_addr);
    if (bind(server_socket, address, address_size) < 0)
    {
        printf("ERROR: Bind \n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Checks whether the SERVER arguments are passed in the correct way
 * @param argscount Number of arguments
 * @param args Array of arguments passed by the user
 */
void check_args(int argscount, char **args)
{
    if (argscount != 4)
    {
        // Port number was not specified from the user, the RFC says that PORT 69 should be used
        port = PORT;
        if (argscount != 2)
        {
            printf("ERROR:Invalid number of arguments\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            directory = args[1];
            if (chdir(directory) < 0)
            {
                printf("ERROR: Invalid directory path passed \n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        directory = args[3];
        if (strcmp(args[1], "-p"))
        {
            printf("ERROR:Expected \"-p\" argument to be here \n");
            exit(EXIT_FAILURE);
        }
        else if (atoi(args[2]) < 0 || atoi(args[2]) > 65635)
        {
            printf("ERROR: Invalid port number\n");
            exit(EXIT_FAILURE);
        }
        else if (chdir(directory) < 0)
        {
            printf("ERROR: Invalid directory path passed \n");
            exit(EXIT_FAILURE);
        }
        // We received valid port number from input->we set the
        port = atoi(args[2]);
    }
}

/**
 * @brief Function used to check whether the size of the file we are about to receive is not bigger than the space left in the directory
 * @param socket Source ID
 * @param address Destination address
 * @param asize Size of the file
 * @param directory_path Path of the directory that is about to be checked
 * @param tsize Size of the file that is to be sent
 * @return True if there is enough space
 */
bool check_dir_space(char *directory_path, unsigned long asize)
{
    struct statvfs st;
    statvfs(directory_path, &st);
    unsigned long free_space = st.f_bfree * st.f_frsize;
    if (asize > free_space)
    {
        return false;
    }
    return true;
}

/**
 * @brief Attaches given string to the options
 * @param str String to attach to the options
 * @param lenght Current lenght of the options
 * @return Lenght of the options
 */
int options_attach(char *str, int lenght, char *opts)
{
    if (lenght == 0)
    {
        strcpy(opts, str);
        lenght += strlen(str);
        return lenght;
    }
    else
    {
        strcpy(opts + lenght + 1, str);
        lenght += strlen(str);
        return lenght + 1;
    }
}
/**
 * @brief Checks and parses the options
 * @param options String of options that needs to be checked and parsed
 * @return True if everything is the way it should be, False if error occurrs
 */
bool parse_options(char *options, char *opts)
{
    int lenght = 0;
    while (options[0] != '\0')
    {
        options = strchr(options, '\0') + 1;
        if (!strcasecmp(options, "blksize"))
        {
            lenght = options_attach(options, lenght, opts);
            options = strchr(options, '\0') + 1;
            if (options[0] == '\0')
            {
                return false;
            }
            else
            {
                blocksize = atoi(options);
                if (blocksize < 8 || blocksize > 65464)
                {
                    return false;
                }
                else
                {
                    lenght = options_attach(options, lenght, opts);
                    continue;
                }
            }
        }
        if (!strcasecmp(options, "timeout"))
        {
            lenght = options_attach(options, lenght, opts);
            options = strchr(options, '\0') + 1;
            if (options[0] == '\0')
            {
                printf("Time optionswrongly passed \n");
                return false;
            }
            else
            {
                timeout = atoi(options);
                if (timeout > 255 || timeout < 1)
                {
                    printf("Time option wrongly passed \n");
                    return false;
                }
                else
                {
                    tv.tv_sec = timeout;
                    lenght = options_attach(options, lenght, opts);
                    continue;
                }
            }
        }
        if (!strcasecmp(options, "tsize"))
        {
            lenght = options_attach(options, lenght, opts);
            options = strchr(options, '\0') + 1;
            if (options[0] == '\0')
            {
                printf("Tsize option wrongly passed \n");
                return false;
            }
            else
            {
                tsize = atoi(options);
                if (tsize <= 0 || !check_dir_space(directory, tsize))
                {
                    printf("Tsize option wrongly passed \n");
                    return false;
                }
                else
                {
                    lenght = options_attach(options, lenght, opts);
                    continue;
                }
            }
        }
    }
    return true;
}

ssize_t send_netascii_data(ssize_t len, socklen_t slen, struct sockaddr *address, char *data, uint16_t block, int socket)
{
    tftp_message *message = malloc(sizeof(tftp_message) + len);
    message->data.opcode = htons(DATA);
    message->data.block_number = htons(block);
    memcpy(message->data.data, data, len);
    ssize_t x;
    if ((x = sendto(socket, message, len + 4, 0, address, slen)) < 0)
    {
        printf("ERROR: sendto()\n");
    }
    free(message);
    return x;
}

/**
 * @brief Function used by SERVER for handling the download process
 * @param socket Source ID
 * @param address Destination address
 * @param slen Adress lenght
 * @param message Structured data type used for storing informations about the messages
 * @param filename The name of the file that we are going to sent
 * @param fd File that we are going to work with
 * @return Number of bytes that have been sent
 */
void server_download(FILE *fd, struct sockaddr *address, socklen_t slen, int socket, char *filename, char *opts, int mode, bool optionsi)
{
    ssize_t x, datalen;
    uint8_t data[blocksize];
    char *ndata = malloc(blocksize);
    uint16_t block = 0;
    tftp_message message;
    int tiktok;
    bool extrach = false;
    char extra;
    if (optionsi)
    {
        send_oack(socket, 0, address, slen, opts);
        x = receive_message(socket, &message, address, &slen, 512);
        if (x != 4 || ntohs(message.opcode) != ACK || ntohs(message.ack.block_number != 0))
        {
            send_error(socket, address, slen, 0, "ERROR: Received wrong response to OACK\n");
            return;
        }
    }
    if (mode == NETASCII)
    {
        fd = fopen(filename, "r");
    }
    else
    {
        fd = fopen(filename, "rb");
    }
    if (fd == NULL)
    {
        send_error(socket, address, slen, file_not_found, "ERROR: File not found\n");
        close(socket);
        free(ndata);
        return;
    }

    while (true)
    {
        if (mode == NETASCII)
        {
            for (int i = 0; i < blocksize; i++)
            {

                if (extrach)
                {
                    ndata[i] = extra;
                    extrach = false;
                    continue;
                }
                datalen = i + 1;
                int c = fgetc(fd);

                if (c == '\n')
                {
                    ndata[i] = '\r';
                    i++;
                    if (i == blocksize)
                    {
                        extra = c;
                        extrach = true;
                        datalen = blocksize;
                        break;
                    }
                    ndata[i] = c;
                    if (i == (blocksize - 1))
                    {
                        datalen = blocksize;
                        break;
                    }
                    continue;
                }
                if (c == '\r')
                {
                    ndata[i] = '\0';
                    i++;
                    if (i == (blocksize - 1))
                    {
                        extra = c;
                        extrach = true;
                        datalen = blocksize;
                        break;
                    }
                    ndata[i] = c;
                    if (i == (blocksize - 1))
                    {
                        datalen = blocksize;
                        break;
                    }
                    continue;
                }
                if (c == EOF)
                {
                    ndata[i] = c;
                    break;
                }
                ndata[i] = c;
            }
        }
        else
        {
            datalen = fread(data, 1, sizeof(data), fd);
        }
        block++;
        for (tiktok = RECV_RETRIES; tiktok; tiktok--)
        {
            if (mode == OCTET)
            {
                x = send_data(datalen, slen, address, data, block, socket);
            }
            else
            {
                x = send_netascii_data(datalen, slen, address, ndata, block, socket);
            }
            if (x < 0)
            {
                close(socket);
                fclose(fd);
                free(ndata);
                return;
            }
            x = receive_message(socket, &message, address, &slen, 512);
            if (x >= 0 && x < 4)
            {
                send_error(socket, address, slen, 0, "ERROR: Received wrong response\n");
                close(socket);
                fclose(fd);
                free(ndata);
                return;
            }
            if (x >= 4)
            {
                break;
            }
            tv.tv_sec *= 2;
            if (errno != EAGAIN)
            {
                close(socket);
                fclose(fd);
                free(ndata);
                return;
            }
        }
        if (!tiktok)
        {
            close(socket);
            fclose(fd);
            free(ndata);
            return;
        }
        if (!opcodes_check_download(&message, block, block, slen, address))
        {
            close(socket);
            fclose(fd);
            free(ndata);
            return;
        }
        // Last packet sent
        if (datalen < blocksize)
        {
            free(ndata);
            fclose(fd);
            return;
        }
    }
}

/**
 * @brief Function used by both SERVER and CLIENT for handling the UPLOAD process
 * @param socket Source ID
 * @param address Destination address
 * @param slen
 * @param message Structured data type used for storing informations about the messages
 * @param filename The name of the file that we are going to sent
 * @param mode Defines which mode is going to be used
 * @param fd File that we are going to work with
 * @return Number of bytes that have been sent
 */
void server_upload(FILE *fd, struct sockaddr *address, socklen_t slen, int socket, char *filename, bool optionsi, char *opts)
{
    bool end = false;
    fd = fopen(filename, "w");
    tftp_message *message = malloc(sizeof(tftp_message) + blocksize);
    ssize_t x;
    uint16_t block = 0;
    if (optionsi)
    {
        send_oack(socket, 0, address, slen, opts);
    }
    else
    {
        x = send_ack(socket, block, address, slen);
    }
    if (x < 0)
    {
        fclose(fd);
        return;
    }
    int tiktok;
    while (true)
    {
        for (tiktok = RECV_RETRIES; tiktok; tiktok--)
        {
            // This should be in loop that is handling timeout
            x = receive_message(socket, message, address, &slen, blocksize);
            if (x >= 0 && x < 4)
            {
                send_error(socket, address, slen, 0, "ERROR: Received wrong response\n");
                free(message);
                close(socket);
                fclose(fd);
                remove(filename);
                return;
            }

            // I guess if we received a correct response on time we jump out of this for that is handling timeout, this break is not for while loop
            if (x >= 4)
            {
                break;
            }

            x = send_ack(socket, block, address, slen);

            if (x < 0)
            {
                // send error
                free(message);
                close(socket);
                fclose(fd);
                remove(filename);
                return;
            }
        }
        if (!tiktok)
        {
            // Transfer timed out
            send_error(socket, address, slen, 0, "ERROR: Timeout\n");
            free(message);
            close(socket);
            fclose(fd);
            remove(filename);
            return;
        }

        block++;
        if (x < blocksize)
        {
            end = true;
        }
        if (!opcodes_check_upload(socket, block, message, slen, address))
        {
            // send error
            free(message);
            close(socket);
            fclose(fd);
            remove(filename);
            return;
        }
        x = fwrite(message->data.data, 1, x - 4, fd);
        if (x < 0)
        {
            // send error
            free(message);
            close(socket);
            fclose(fd);
            remove(filename);
            return;
        }
        x = send_ack(socket, block, address, slen);
        if (x < 0)
        {
            free(message);
            close(socket);
            fclose(fd);
            remove(filename);
            return;
        }
        if (end)
        {
            // send error
            free(message);
            fclose(fd);
            return;
        }
    }
}

/**
 * @brief Handles the client requests
 * @param address Destination address
 * @param len
 * @param msg Structured data type used for storing informations about the messages
 * @param lenght
 */
void handle_client_rqst(tftp_message_request *msg, struct sockaddr *adress, socklen_t len, ssize_t lenght, int socket)
{
    tv.tv_usec = 0;
    tv.tv_sec = RECV_TIMEOUT;
    char *filename, *mode, *lastnode;
    int client_socket;
    uint16_t opcode;
    FILE *fd;
    char *options;
    bool optinfo = false;
    client_socket = create_socket();
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        printf("setsockopt()\n");
        free(msg);
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    filename = msg->request.filename_and_mode;
    mode = strchr(filename, '\0') + 1;
    ssize_t fmlen = strlen(mode) + strlen(filename);
    char *opts = NULL;
    if (fmlen != (lenght - 4))
    {
        opts = (char *)malloc(lenght - 4 - fmlen);
        options = mode;
        if (!parse_options(options, opts))
        {
            send_error(client_socket, adress, len, 8, "ERROR: Options passed in an incorrect way\n");
            close(client_socket);
            free(opts);
            free(msg);
            return;
        }
        optinfo = true;
    }
    else
    {
        lastnode = &filename[lenght - 3];
        if (*lastnode != '\0')
        {
            send_error(client_socket, adress, len, 0, "ERROR: Filename and mode passed in an incorrect way\n");
            free(opts);
            close(client_socket);
            free(msg);
            return;
        }
    }
    int transfermode;
    if (!strcmp(mode, "octet"))
    {
        transfermode = OCTET;
    }
    else
    {
        if (!strcmp(mode, "netascii"))
        {
            transfermode = NETASCII;
        }
        else
        {
            send_error(client_socket, adress, len, 0, "ERROR: Invalid mode specified \n");
            free(opts);
            close(client_socket);
            free(msg);
            return;
        }
    }

    if (filename[0] == '/' && strncmp(filename, directory, strlen(directory)) != 0)
    {
        send_error(client_socket, adress, len, 0, "ERROR: Filename outside base directory \n");
        free(opts);
        close(client_socket);
        free(msg);
        return;
    }
    opcode = ntohs(msg->opcode);
    if (opcode == RRQ)
    {
        server_download(fd, adress, len, client_socket, filename, opts, transfermode, optinfo);
        free(msg);
    }
    else if (opcode == WRQ)
    {
        struct stat file_info;
        if (stat(filename, &file_info) == 0)
        {
            send_error(client_socket, adress, len, file_exists, "ERROR: Filename already exists \n");
            free(opts);
            close(client_socket);
            free(msg);
            return;
        }
        server_upload(fd, adress, len, client_socket, filename, optinfo, opts);
        free(msg);
    }
    close(client_socket);
    free(opts);
    return;
}

/**
 * @brief Server function
 * @param socket Source ID
 */
void server(int sck)
{
    struct sockaddr_in client_addr;
    socklen_t addr_size;
    struct sockaddr *addr = (struct sockaddr *)&client_addr;
    const char *hostaddrp;
    struct hostent *hostp;
    while (1)
    {
        tftp_message_request *msg = malloc(sizeof(tftp_message) + 512);
        uint16_t opcode;
        ssize_t lenght;
        addr_size = sizeof(client_addr);
        if ((lenght = receive_message_request(sck, msg, addr, &addr_size)) < 4)
        {
            printf("ERROR: Invalid message received\n");
        }
        hostp = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr, sizeof(client_addr.sin_addr.s_addr), AF_INET);
        hostaddrp = inet_ntoa(client_addr.sin_addr);
        opcode = ntohs(msg->opcode);
        if (opcode == WRQ || opcode == RRQ)
        {
            if (fork() == 0)
            {
                handle_client_rqst(msg, addr, addr_size, lenght, sck);
            }
            else
            {
                close(sck);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("Invalid opcode received\n");
        }
    }
    close(sck);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Main function
 * @param argc number of arguments
 * @param argv array of arguments
 * @return 0 if everything is okay, 1 if error occurrs during the programme
 */
int main(int argc, char *argv[])
{
    check_args(argc, argv);
    int socket = create_socket();
    server_bind(socket);
    server(socket);

    return 0;
}