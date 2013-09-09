#ifndef FIFO_SERVER_HEADER
#define FIFO_SERVER_HEADER

#define CLIENT_FIFO_SIZE        21
#define CLIENT_FIFO_TEMPLATE    "/tmp/fifo_cl.%ld"
#define SERVER_PATHNAME         "/tmp/fifo_sv"

typedef struct  {
    pid_t pid;
    int data_len;
} Request;

typedef struct  {
    int data_len;
} Response;

#endif
