#ifndef DISPATCHER_HEADER
#define DISPATCHER_HEADER

#include <sys/socket.h>

#define     MAX_DATA_SIZE   (1024*4)
#define     PORT_NUM        "50000"
#define     BACKLOG         5
#define     ADDRSTRLEN      (NI_MAXHOST + NI_MAXSERV + 10)

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
