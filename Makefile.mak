CC = gcc
CFLAGS= -Wall -g
LIB = -pthread

all: p3-dogServerPipe p3-dogClient 

p3-dogServerPipe: p3-dogServerPipe.c
	$(CC) -o ./bin/$@ $^ $(LIB)

p3-dogClient: p3-dogClient.c
	$(CC) -o ./bin/$@ $^ 






