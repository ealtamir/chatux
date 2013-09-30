#ifndef DISPATCHER_HEADER
#define DISPATCHER_HEADER

#include <sys/socket.h>

typedef struct {
    int msg_size;
    int pipe_fd[2];
} ThreadMsgHeader;

typedef struct {
    int cfd;
    char *host;
    char *service;
    const struct sockaddr *sa;
    socklen_t salen;
} ThreadData;

#endif
