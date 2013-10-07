#ifndef DISPATCHER_HEADER
#define DISPATCHER_HEADER

#include <sys/socket.h>

#define     MAX_DATA_SIZE   (1024*4)
#define     PORT_NUM        "50003"
#define     BACKLOG         5
#define     ADDRSTRLEN      (NI_MAXHOST + NI_MAXSERV + 10)

typedef struct {
    int msg_size;
    int thread_id;
} ThreadMsgHeader;

typedef struct {
    int cfd;
    int thread_id;
    char *host;
    char *service;
    const struct sockaddr *sa;
    socklen_t salen;
} ThreadData;

#endif
