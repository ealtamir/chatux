CC=gcc
CFLAGS=-c
VPATH=src:lib:etc:bin
TARGET=client
CLIENT=client
SERVER=server
.RECIPEPREFIX =    

# Automatic Variables
# $^ : list of all prerequisites
# $< : first prerequisite
# $@ : name of the target

ALL: bin/all clean

bin/all: get_num.o helpers.o error_functions.o fifo_client.o fifo_server.o sock_svr.o
	$(CC) $(@D)/helpers.o $(@D)/error_functions.o $(@D)/get_num.o $(@D)/sock_svr.o -o $(@D)/sock_svr
	$(CC) $(@D)/helpers.o $(@D)/error_functions.o $(@D)/fifo_$(SERVER).o -o $(@D)/$(SERVER)
	$(CC) $(@D)/helpers.o $(@D)/error_functions.o $(@D)/fifo_$(CLIENT).o -o $(@D)/$(CLIENT)

sock_svr.o: sock_svr.c
	$(CC) $(CFLAGS) $< -o bin/$@

error_functions.o: error_functions.c error_functions.h
	$(CC) $(CFLAGS) $< -o bin/$@

fifo_$(CLIENT).o: fifo_$(CLIENT).c fifo_$(CLIENT).h
	$(CC) $(CFLAGS) $< -o bin/$@

fifo_$(SERVER).o: fifo_$(SERVER).c fifo_$(SERVER).h
	$(CC) $(CFLAGS) $< -o bin/$@

helpers.o: helpers.c helpers.h
	$(CC) $(CFLAGS) $< -o bin/$@

get_num.o: get_num.c get_num.h
	$(CC) $(CFLAGS) $< -o bin/$@

clean:
	rm bin/*.o

