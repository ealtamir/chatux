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

bin/all: get_num.o helpers.o error_functions.o dispatcher.o server.o
	$(CC) $(@D)/helpers.o $(@D)/error_functions.o $(@D)/get_num.o $(@D)/server.o -o $(@D)/server
	$(CC) $(@D)/helpers.o $(@D)/error_functions.o $(@D)/get_num.o $(@D)/dispatcher.o -o $(@D)/dispatcher

server.o: server.c server.h
	$(CC) $(CFLAGS) $< -o bin/$@

dispatcher.o: dispatcher.c dispatcher.h
	$(CC) $(CFLAGS) $< -o bin/$@

error_functions.o: error_functions.c error_functions.h
	$(CC) $(CFLAGS) $< -o bin/$@

helpers.o: helpers.c helpers.h
	$(CC) $(CFLAGS) $< -o bin/$@

get_num.o: get_num.c get_num.h
	$(CC) $(CFLAGS) $< -o bin/$@

clean:
	rm bin/*.o

