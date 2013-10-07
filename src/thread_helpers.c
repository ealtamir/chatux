#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>

#include "../lib/common_headers.h"
#include "../lib/dispatcher.h"
#include "../lib/error_functions.h"
#include "../lib/get_num.h"
#include "../lib/thread_helpers.h"
#include "../lib/server.h"

void freeThreadData(ThreadData *td);

pthread_mutex_t fifo_mtx = PTHREAD_MUTEX_INITIALIZER;


void* toThreadDelegator(void *args) {

    ThreadData *td = (ThreadData*) args;
    char client_data[MAX_DATA_SIZE];
    int data_len = 0;
    int val = -1;
    int svr_fifo = 0;
    int pipe_fd[2];
    void *svr_data = NULL; // malloc ptr for getdatafromserver.

    fprintf(stdout, "Thread created - (%s, %s)\n", td->host, td->service);

    val = readClientData(td->cfd, client_data, MAX_DATA_SIZE);
    if (val == -1)
        errMsg("Error when reading data from a client");

    fprintf(stdout, "val: %d.\n", val);

    if (val != -1) {
        data_len = strlen(client_data);

        val = sendDataToServer(client_data, data_len, pipe_fd);
        printf("Data sent to server: %s", client_data);
        if (val == -1)
            errMsg("Error while sending data to server.");
    }


    if (val != -1) {
        val = getDataFromServer(pipe_fd, svr_data);
        if (val != -1 || val != sizeof(ThreadMsgHeader))
            errMsg("Error when receiving response from server in thread.");
    }

    fprintf(stdout, "Data received from server: %s", svr_data);

    if (val != -1) {
        val = sendDataToClient(td, svr_data);
        if (val != 0)
            errMsg("Error when sending data to client.");
    }

    fprintf(stdout, "data to client: %s.\n", (char*) svr_data);

    // Free ptr before starting new listening cycle.
    free(svr_data);

    val = startIOListener(td, pipe_fd);
    if (val == -1) {
        // Some error checking code.
    }

    //
    // Cleaning operations
    //
    close(td->cfd);         // Close client socket
    freeThreadData(td);

    return NULL;
}

void freeThreadData(ThreadData *td) {
    free(td->host);
    free(td->service);
    free(td);
}

int readClientData(int cfd, char *data_buffer, int data_len) {

    int index;
    int val = -1;

    for(index = 0; val != 0 && index < data_len; index++) {

        val = read(cfd, &data_buffer[index], 1);

        if (val == 1) {
            if (data_buffer[index] == '\n') {
                data_buffer[index] = '\0';
                break;
            }
        } else if (val == -1) {
            errMsg("Thread failed in a read operation.");
            return -1;
        }
    }

    return index;
}

int getDataFromServer(int *pipe_fd, void *svr_data) {

    int val = -1;
    ThreadMsgHeader t_header;

    fprintf(stdout, "Dispatcher: Start reading from pipe.\n");

    close(pipe_fd[1]);

    //
    // Thread hasn't closed his pipe write side fd, it
    // should be done by the server.
    //
    val = read(pipe_fd[0], &t_header, sizeof(ThreadMsgHeader));
    if (val != sizeof(ThreadMsgHeader)) {
        errMsg("Dispatcher: Error when thread tried to read from pipe: header size mismatch.");
        return -1;
    }

    svr_data = malloc(t_header.msg_size);
    val = read(pipe_fd[0], svr_data, t_header.msg_size);
    if (val != t_header.msg_size) {
        errMsg("Dispatcher: Error when thread tried to read from pipe: data size mismatch.");
        free(svr_data);
        return -1;
    }


    return 0;
}

int sendDataToServer(char *client_data, int data_len, int *pipe_fd) {

    ThreadMsgHeader t_header;
    int fifo_fd = -1;
    ssize_t val = -1;

    fifo_fd = open(SERVER_FIFO_PATHNAME, O_WRONLY);
    if (fifo_fd == -1) {
        errMsg("Error when opening server FIFO.");
        return -1;
    }

    val = pipe(pipe_fd);
    if (val == -1) {
        errMsg("Error when opening thread pipe.");
        return -1;
    }

    t_header.msg_size = data_len;
    t_header.pipe_fd[1] = pipe_fd[1];
    t_header.pipe_fd[0] = pipe_fd[0];

    close(t_header.pipe_fd[1]);

    //
    // Lock the mutex.
    //
    pthread_mutex_lock(&fifo_mtx);

    fprintf(stdout, "Pipe file descriptor: read %d - write %d\n", pipe_fd[0], pipe_fd[1]);
    //fprintf(stdout, "Dispatcher: Writing data to server.\n");

    val = write(fifo_fd, &t_header, sizeof(ThreadMsgHeader));
    if (val != sizeof(ThreadMsgHeader)) {
        errMsg("Error when writing header to server FIFO: size mismatch.");
        pthread_mutex_unlock(&fifo_mtx); // If failure unlock mutex.
        return -1;
    }

    val = write(fifo_fd, client_data, data_len);
    if (val != data_len) {
        errMsg("Error when writing client data to server FIFO: size mismatch.");
        pthread_mutex_unlock(&fifo_mtx); // If failure unlock mutex.
        return -1;
    }

    //
    // Unlock the mutex.
    //
    pthread_mutex_unlock(&fifo_mtx);

    fprintf(stdout, "Dispatcher: Data sent to server.\n");

    return 0;
}

int sendDataToClient(ThreadData *td, void *svr_data) {
    int val = -1;
    int len = 0;

    len = strlen((char*) svr_data);
    val = write(td->cfd, svr_data, len);
    if (val != len) {
        errMsg("Thread couldn't send message to client: data size mismatch");
        return -1;
    }

    return 0;
}

int startIOListener(ThreadData *td, int *pipe_fd) {

    fd_set readfds;
    int ready = -1;
    int nfds = 0;

    for(;;) {
        FD_ZERO(&readfds);
        FD_SET(td->cfd, &readfds);
        FD_SET(pipe_fd[1], &readfds);

        nfds = max(td->cfd, pipe_fd[1]) + 1;

        ready = select(nfds, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            errMsg("Error when using select() inside a thread.");
            break;
        }

        if (FD_ISSET(td->cfd, &readfds)) {
            fprintf(stdout, "Received new input from socket.\n");
        }
        if (FD_ISSET(pipe_fd[1], &readfds)) {
            fprintf(stdout, "Received new input from pipe.\n");
        }

    }

    return (ready == -1)? -1: 0;
}
