CC=gcc
CFLAGS=-c
VPATH=src:lib:etc:bin
.RECIPEPREFIX =    

# Automatic Variables
# $^ : list of all prerequisites
# $< : first prerequisite
# $@ : name of the target

bin/exec: error_functions.o fifo_server.o
	$(CC) $(@D) helpers.o $(@D)/error_functions.o $(@D)/fifo_server.c.o -o $@

error_functions.o: error_functions.c error_functions.h
	$(CC) $(CFLAGS) $< -o bin/$@

fifo_server.o: pipe_test.c
	$(CC) $(CFLAGS) $< -o bin/$@

helpers.o: helpers.c helpers.h
	$(CC) $(CFLAGS) $< -o bin/$@

clean:
	rm bin/*.o

