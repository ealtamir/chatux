#include <signal.h>
#include <libgen.h>

#include "../lib/helpers.h"
#include "../lib/common_headers.h"
#include "../lib/dispatcher.h"
#include "../lib/error_functions.h"
#include "../lib/server.h"
#include "../lib/thread_helpers.h"

#define     DISPATCHER_PATHNAME     "/bin/dispatcher"
#define     DISPATCHER_NAME         "dispatcher"

int initDispatcher();
int startListening(int fifo_fd);
int processRequest(ThreadMsgHeader t_header, char *user_data, int data_len);

int main(int argc, const char *argv[])
{
    int val = -1;
    int fifo_fd     = -1;
    int dummy_fd    = -1;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExitEN(errno, "Failed to ignore SIGPIPE signal");

    fprintf(stdout, "Starting server...\n");

    val = mkfifo(SERVER_FIFO_PATHNAME, S_IRUSR | S_IWUSR | S_IWGRP);
    if (val == -1 && errno != EEXIST)
        errExitEN(errno, "Failed to create server FIFO");

    fifo_fd = open(SERVER_FIFO_PATHNAME, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1)
        errExitEN(errno, "Failed to open the server fifo");

    dummy_fd = open(SERVER_FIFO_PATHNAME, O_WRONLY);
    if (dummy_fd == -1)
        errExitEN(errno, "Failed to open dummy_fifo for writing");

    fprintf(stdout, "Initializing dispatcher...\n");

    val = initDispatcher();
    if (val == -1)
        errExitEN(errno, "Failed to initialize dispatcher.");

    fprintf(stdout, "Dispatcher initialized. Starting service loop...\n");

    //
    // Main server loop
    //
    val = startListening(fifo_fd);
    if (val == -1)
        errExitEN(errno, "Server failed in the listening loop.");

    close(fifo_fd);
    close(dummy_fd);

    return 0;
}

int processRequest(ThreadMsgHeader t_header, char *user_data, int data_len) {

    int val = 0;
    int thr_fifo_fd = 0;
    char thr_fifo_path[MAX_FIFO_N_LEN];
    ThreadMsgHeader resp_header;

    fprintf(stdout, "Server: thread_id: %d\n", t_header.thread_id);
    snprintf(thr_fifo_path, MAX_FIFO_N_LEN, T_FIFO_PATH, t_header.thread_id);

    thr_fifo_fd = open(thr_fifo_path, O_WRONLY);
    if (thr_fifo_fd == -1) {
        errExitEN(errno, "Server: Couldn't open thread fifo for writing");
    }

    resp_header.msg_size = data_len;
    val = write(thr_fifo_fd, &resp_header, sizeof(ThreadMsgHeader));
    if (val != sizeof(ThreadMsgHeader)) {
        errMsg("Server: couldn't send response HEADER through fifo: size mismatch. val = %d, expected %d", val, sizeof(ThreadMsgHeader));
        return -1;
    }

    val = write(thr_fifo_fd, user_data, data_len);
    if (val != data_len) {
        errMsg("Server: couldn't send response DATA through fifo: size mismatch");
        return -1;
    }

    fprintf(stdout, "Server: Data sent: %s\n", user_data);

    close(thr_fifo_fd);

    return 0;
}

int startListening(int fifo_fd) {
    int val = -1;
    int flags = 0;
    ThreadMsgHeader t_header;
    char *request_data = NULL;

    fprintf(stdout, "Server: Listening for dispatcher requests....\n");

    setBlocking(fifo_fd);

    for(;;) {
        val = read(fifo_fd, &t_header, sizeof(ThreadMsgHeader));
        if (val != sizeof(ThreadMsgHeader)) {
            errMsg("Error while reading server header.");
            continue;
        }

        fprintf(stdout, "Server: request header received - msg_size: %d.\n", t_header.msg_size);

        request_data = (char*) malloc(t_header.msg_size);
        if (request_data == NULL) {
            errMsg("Server malloc failed with - %s", errno);
            continue;
        }

        val = read(fifo_fd, request_data, t_header.msg_size);
        if (val != t_header.msg_size) {
            errMsg("Error while reading request_data");
            free(request_data);
            continue;
        }

        val = processRequest(t_header, request_data, t_header.msg_size);

        free(request_data);
    }

    return 0;
}

int initDispatcher() {
    pid_t child_id = 0;
    int result = 0;
    char *argV[10];
    char *cwd;

    // Build arguments for dispatcher.
    argV[0] = DISPATCHER_NAME;
    argV[1] = SERVER_FIFO_PATHNAME;
    argV[2] = NULL;

    cwd = malloc(100);

    getcwd(cwd, 100);
    strcat(cwd, DISPATCHER_PATHNAME);

    fprintf(stdout, "dispatcher path: %s.\n", cwd);

    switch (child_id = fork()) {
        case 0:
            result = execv((const char*) cwd, argV);
            errExitEN(errno, "Could not execute dispatcher program");
            break;
        case -1:
            result = -1;
            break;
        default:
            result = 0;
            fprintf(stdout, "Dispatcher PID: %d\n", child_id);
            break;
    }

    free(cwd);

    return result;

}
