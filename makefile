CC=gcc
CFLAGS=-c
VPATH=src:lib:etc:bin
.RECIPEPREFIX =    

# Automatic Variables
# $^ : list of all prerequisites
# $< : first prerequisite
# $@ : name of the target

bin/exec: error_functions.o pipe_test.o
	$(CC) $(@D)/error_functions.o $(@D)/pipe_test.o -o $@

error_functions.o: error_functions.c error_functions.h
	$(CC) $(CFLAGS) $< -o bin/$@

pipe_test.o: pipe_test.c
	$(CC) $(CFLAGS) $< -o bin/$@

clean:
	rm bin/*.o

