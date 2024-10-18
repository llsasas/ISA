CC = gcc
CFLAGS = -Wall

SRC_DIR = src
SERVER = tftp-server
CLIENT = tftp-client

SERVER_SRC = $(SRC_DIR)/tftp-server.c $(SRC_DIR)/messages.c
CLIENT_SRC = $(SRC_DIR)/tftp-client.c $(SRC_DIR)/messages.c

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC) $(SRC_DIR)/tftp-server.h $(SRC_DIR)/messages.h
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC) $(SRC_DIR)/tftp-client.h $(SRC_DIR)/messages.h
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRC)

clean:
	rm -f $(SERVER) $(CLIENT)