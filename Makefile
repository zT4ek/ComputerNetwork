CC = gcc
TARGET = server
CFLAGS = -W -Wall

server: server.c
				$(CC) -g -o server server.c


clean:
				rm $(TARGET)
