#include <fcntl.h>
#include <signal.h>
#include "../lib/fifo_server.h"
#include "../lib/common_headers.h"
#include "../lib/error_functions.h"
#include "../lib/helpers.h"


#define INFINITE_LOOP   1

int main(int argc, const char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "--help") == 0)
        usageErr("%s string\n", argv[0]);

    Request cl_request;
    Response sv_response;
    char client_fifo[CLIENT_FIFO_SIZE];
    char* req_buffer = 0;
    int dummyfd;
    int serverfd, clientfd;
    int request_num = 0;

    serverfd = dummyfd = clientfd = -1;
    umask(0); // Set process file permissions to 000.

    if (mkfifo(SERVER_PATHNAME, S_IRUSR | S_IWUSR | S_IWGRP) == -1
            && errno != EEXIST)
        errExit("Failed to create server fifo");

    serverfd = open(SERVER_PATHNAME, O_RDONLY);
    if (serverfd == -1)
        errExit("failed to open created FIFO");

    // Open dummy FIFO write to prevent seeing EOF.
    dummyfd = open(SERVER_PATHNAME, O_WRONLY);
    if (dummyfd == -1)
        errExit("failed to open dummy FIFO writer");

    // Ignore the SIGPIPE signal if writing to a FIFO
    // with no reader.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExit("Failed to set correct signal configuration.");

    // Server can only be stopped with CTRL-C signal.
    while(INFINITE_LOOP) {
        if (read(serverfd, &cl_request, sizeof(Request)) != sizeof(Request)) {
            errMsg("Error while reading request #%d.\n", request_num);
            continue; // If what was read doesn't match
        }

        req_buffer = malloc(cl_request.data_len);
        if (read(serverfd, req_buffer, cl_request.data_len) == -1) {
            free(req_buffer);
            errMsg("Error while trying to read data from client.\n");
            continue;
        }

        rot13(req_buffer, req_buffer, cl_request.data_len);

        snprintf(client_fifo, CLIENT_FIFO_SIZE,
                CLIENT_FIFO_TEMPLATE, (long int)cl_request.pid);
        fprintf(stdout, "Sending to client: %ld\n",
                (long int)cl_request.pid);

        clientfd = open(client_fifo, O_WRONLY);
        if (clientfd == -1) {
            errMsg("Failed to open client fifo: %ld",
                (long int) cl_request.pid);
            continue;
        }
        sv_response.data_len = cl_request.data_len;
        if (write(clientfd, &sv_response, sizeof(Response))
                != sizeof(Response))
            fprintf(stderr, "Failed to write %s.\n", client_fifo);
        if (close(clientfd) == -1)
            errMsg("Failed to close fd for client FIFO: %s", client_fifo);

        free(req_buffer); // free memory allocated to send msg to client.
    }
}
