#include <signal.h>

#include "../lib/common_headers.h"
#include "../lib/error_functions.h"
#include "../lib/server.h"
#include "../lib/dispatcher.h"

#define     DISPATCHER_PATHNAME     "dispatcher"
#define     DISPATCHER_NAME         "dispatcher"

int initDispatcher();
int startListening();
int processRequest(ThreadMsgHeader t_header, char *user_data, int data_len);

int main(int argc, const char *argv[])
{
    int val = -1;
    int fifo_fd     = -1;
    int dummy_fd    = -1;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExitEN(errno, "Failed to ignore SIGPIPE signal");

    val = mkfifo(SERVER_FIFO_PATHNAME, S_IRUSR | S_IWUSR | S_IWGRP);
    if (val == -1 && errno != EEXIST)
        errExitEN(errno, "Failed to create server FIFO");

    fifo_fd = open(SERVER_FIFO_PATHNAME, O_RDONLY);
    if (fifo_fd == -1)
        errExitEN(errno, "Failed to open the server fifo");

    dummy_fd = open(SERVER_FIFO_PATHNAME, O_WRONLY);
    if (dummy_fd == -1)
        errExitEN(errno, "Failed to open dummy_fifo for writing");

    val = initDispatcher();
    if (val == -1)
        errExitEN(errno, "Failed to initialize dispatcher.");

    //
    // Main server loop
    //
    val = startListening();
    if (val == -1)
        errExitEN(errno, "Server failed in the listening loop.");

    close(fifo_fd);
    close(dummy_fd);

    return 0;
}

int processRequest(ThreadMsgHeader t_header,
        char *user_data, int data_len) {

    int val = 0;
    ThreadMsgHeader resp_header;

    resp_header.msg_size = data_len;
    val = write(t_header.pipe_fd[1], &resp_header, sizeof(ThreadMsgHeader));
    if (val != sizeof(ThreadMsgHeader)) {
        errMsg("Server couldn't send response header through pipe: size mismatch.");
        return -1;
    }

    val = write(t_header.pipe_fd[1], user_data, data_len);
    if (val != data_len) {
        errMsg("Server couldn't send response header through pipe: size mismatch");
        return -1;
    }

    //
    // SHOULD NOT BE CLOSED UNTIL THREAD IS KILLED.
    //
    close(t_header.pipe_fd[1]);

    fprintf(stdout, "%s\n", user_data);

    return 0;
}

int startListening(int fifo_fd) {
    int val = -1;
    ThreadMsgHeader t_header;
    char *request_data = NULL;

    for(;;) {
        val = read(fifo_fd, &t_header, sizeof(ThreadMsgHeader));
        if (val != sizeof(ThreadMsgHeader)) {
            errMsg("Error while reading server header.");
            continue;
        }

        request_data = malloc(t_header.msg_size);
        if (request_data == NULL)
            errMsg("Server malloc failed with - %s", errno);

        val = read(fifo_fd, &request_data, t_header.msg_size);
        if (val != t_header.msg_size) {
            errMsg("Error while reading request_data");
            free(request_data);
            continue;
        }

        // TODO: FIX THIS!!!!
        //  Close read part of the thread pipe.
        close(t_header.pipe_fd[0]);

        processRequest(t_header, request_data, t_header.msg_size);

        free(request_data);
    }

    return 0;
}

int initDispatcher() {
    pid_t child_id = 0;
    int result = 0;
    char *argV[10];

    // Build arguments for dispatcher.
    argV[0] = DISPATCHER_NAME;
    argV[1] = SERVER_FIFO_PATHNAME;
    argV[2] = NULL;

    switch (child_id = fork()) {
        case 0:
            result = execve(DISPATCHER_PATHNAME, argV, NULL);
            break;
        case -1:
            result = -1;
            break;
        default:
            result = 0;
            break;
    }

    return result;

}
